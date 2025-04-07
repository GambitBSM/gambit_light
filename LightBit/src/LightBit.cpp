//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Functions of module LightBit
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author Anders Kvellestad
///          (anders.kvellestad@fys.uio.no)
///  \date 2022 Sep, 2023 Apr, 2023 May
///
///  *********************************************

#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/LightBit/LightBit_rollcall.hpp"
#include "gambit/LightBit/LightBit_types.hpp"
#include "gambit_light_interface.h"

namespace Gambit
{
  namespace gambit_light_interface
  {
    // Functions from the gambit_light_interface library
    extern double run_user_loglike(const std::string&, const std::vector<std::string>&, const std::vector<double>&, std::map<std::string,double>&, std::vector<std::string>&);
    extern void init_user_lib_C_CXX_Fortran(const std::string&, const std::string&, const std::string&, const std::string&, const std::vector<std::string>&);
    extern void init_user_lib_Python(const std::string&, const std::string&, const std::string&, const std::vector<std::string>&);
  }
}


namespace Gambit
{
  namespace LightBit
  {
    using namespace LogTags;

    //************************************************************

    /// \name Module-level variables
    /// @{

    std::vector<std::string> listed_user_pars;
    std::vector<std::string> listed_model_pars;
    std::map<std::string,std::string> user_to_model_par_names;
    std::vector<std::string> user_loglikes;

    /// @}



    /// \name Helper functions
    /// @{

    // Get the permutation needed to sort a vector containing 
    // parameter names "p1", "p2", ...
    std::vector<int> get_model_pars_sort_permutation(const std::vector<std::string>& vec)
    {
      std::vector<int> indices(vec.size());
      std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...
      std::sort(indices.begin(), indices.end(),
          [&](const int& a, const int& b) 
          {
            int int_a = std::stoi(vec[a].substr(1));  // E.g. "p10" --> 10
            int int_b = std::stoi(vec[b].substr(1));  // E.g. "p100" --> 100
            return (int_a < int_b);
          }
      );
      return indices;
    }

    // Apply a given permutation to a vector of strings.
    void apply_permutation(std::vector<std::string>& vec, const std::vector<int>& indices)
    {
      std::vector<std::string> temp_vec(vec.size());
      for (std::size_t i = 0; i < vec.size(); i++) 
      {
        temp_vec[i] = vec[indices[i]];
      }
      vec = temp_vec;
    }

    /// @}



    /// \name Module functions
    /// @{

    // The first time this function is run, it performs a series of consistency 
    // checks for the YAML file settings and extracts the information needed.
    void initialisation(bool& result)
    {
      using namespace Pipes::initialisation;

      // TODO: Currently the "inputs" YAML sections are read, but
      // this info is not used for anything. We need to use it to 
      // call each loglike with the correct subset of inputs when 
      // called from the "ouput" module function.

      // Perform the initialisation the first time this function is run.
      static bool initialisation_done = false;
      if (not initialisation_done)
      {
        std::string user_lib;
        std::string loglike_name;
        std::string func_name;
        std::string lang;
        std::vector<std::string> all_outputs;

        YAML::Node userModelNode = runOptions->getNode("UserModel");
        YAML::Node userLogLikesNode = runOptions->getNode("UserLogLikes");

        // Due to the yaml-cpp iterator bug discussed here https://github.com/jbeder/yaml-cpp/issues/833, 
        // we will need some iterator variables we can "manually" increment while looping over YAML nodes.
        // (We use this workaround: https://github.com/jbeder/yaml-cpp/issues/833#issuecomment-943794962 )
        YAML::const_iterator it1;
        YAML::const_iterator it2;
        std::size_t size1;
        std::size_t size2;

        // Check for unknown options or typos in the "UserLogLikes" section.
        const static std::vector<std::string> known_userloglike_options = {"lang", "user_lib", "func_name", "input", "output"};

        it1 = userLogLikesNode.begin();
        size1 = userLogLikesNode.size();
        for (std::size_t i = 0; i < size1; ++i)
        {
          std::string loglike_name = it1->first.as<std::string>();
          const YAML::Node userLogLikesEntry = it1->second;

          it2 = userLogLikesEntry.begin();
          size2 = userLogLikesEntry.size();
          for (std::size_t j = 0; j < size2; ++j)
          {
            std::string option_name = it2->first.as<std::string>();
            if (std::find(known_userloglike_options.begin(), known_userloglike_options.end(), option_name) == known_userloglike_options.end())
            {
              LightBit_error().raise(LOCAL_INFO,
                "Error while parsing the UserLogLikes settings: Unknown option '" 
                + option_name + "' for the loglike '" + loglike_name + "'. "
                "(Maybe a typo?)"
              );
            }
            ++it2;
          }
          ++it1;
        }

        // Collect all parameter names listed in the "UserModel" section (and check for duplicates).
        it1 = userModelNode.begin();
        size1 = userModelNode.size();
        for (std::size_t i = 0; i < size1; ++i)
        {
          // Get the UserModel parameter name
          std::string model_par_name = it1->first.as<std::string>();

          // If there's a "name" entry for this parameter, use it.
          // If not, just use the model_par_name.
          std::string user_par_name = model_par_name;
          const YAML::Node par_node = it1->second;
          if (par_node["name"].IsDefined())
          {
            user_par_name = par_node["name"].as<std::string>();
          }

          // Check for duplicate entries of model_par_name ("p1", "p2", etc.)
          if (std::find(listed_model_pars.begin(), listed_model_pars.end(), model_par_name) != listed_model_pars.end())
          {
            LightBit_error().raise(LOCAL_INFO, 
              "Error while parsing the UserLogLikes settings: Multiple entries " 
              "for the parameter '" + model_par_name + "'."
            );
          }

          // Check for duplicate entries of the user-specified parameter name.
          if (std::find(listed_user_pars.begin(), listed_user_pars.end(), user_par_name) != listed_user_pars.end())
          {
            LightBit_error().raise(LOCAL_INFO, 
              "Error while parsing the UserLogLikes settings: "
              "The parameter name '" + user_par_name + "' is "
              "assigned to multiple parameters."
            );
          }

          // This is a new parameter. Register it.
          listed_user_pars.push_back(user_par_name);
          listed_model_pars.push_back(model_par_name);
          user_to_model_par_names[user_par_name] = model_par_name;

          ++it1;
        }

        // Sort listed_model_pars in the order "p0", "p1" , ..., "p99", "p100", ...
        // Apply the same permutation to the listed_user_pars vector.
        std::vector<int> indices = get_model_pars_sort_permutation(listed_model_pars);
        apply_permutation(listed_model_pars, indices);
        apply_permutation(listed_user_pars, indices);


        // Check consistency and collect settings from the "UserLogLikes" section.
        it1 = userLogLikesNode.begin();
        size1 = userLogLikesNode.size();
        for (std::size_t i = 0; i < size1; ++i)
        {
          std::string loglike_name = it1->first.as<std::string>();
          const YAML::Node userLogLikesEntry = it1->second;

          std::vector<std::string> inputs;
          std::vector<std::string> outputs;

          if (not userLogLikesEntry["user_lib"].IsDefined())
          {
            LightBit_error().raise(LOCAL_INFO, 
              "Error while parsing the UserLogLikes settings: "
              "'user_lib' not specified in config file."
            );
          }
          user_lib = userLogLikesEntry["user_lib"].as<std::string>();

          if (not userLogLikesEntry["func_name"].IsDefined())
          {
            LightBit_error().raise(LOCAL_INFO,
              "Error while parsing the UserLogLikes settings: "
              "'func_name' not specified in config file."
            );
          }
          func_name = userLogLikesEntry["func_name"].as<std::string>();

          if (not userLogLikesEntry["lang"].IsDefined())
          {
            LightBit_error().raise(LOCAL_INFO, 
              "Error while parsing the UserLogLikes settings: "
              "'lang' not specified in config file."
            );
          }
          lang = userLogLikesEntry["lang"].as<std::string>();

          // Avoid confusion with "Fortran" vs "fortran", etc. 
          std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);

          // Treat "lang: cpp" and "lang: cxx" as equivalent to "lang: c++"
          if (lang == "cpp" or lang == "cxx") { lang = "c++"; }

          if (lang != "fortran" and 
              lang != "c" and
              #ifdef HAVE_PYBIND11
                lang != "python" and
              #endif
              lang != "c++")
          {
            LightBit_error().raise(LOCAL_INFO, 
              "Error while parsing the UserLogLikes settings: "
              "unsupported plugin language '" + lang + "'."
            );
          }

          if (userLogLikesEntry["input"].IsDefined())
          {
            const YAML::Node input_node = userLogLikesEntry["input"];
            size2 = input_node.size();
            for (std::size_t j = 0; j < size2; j++)
            {
              std::string input_par_name = input_node[j].as<std::string>();

              // Check for duplicate parameter names in the "input" section.
              bool in_inputs = std::find(inputs.begin(), inputs.end(), input_par_name) != inputs.end();
              if(in_inputs)
              {
                LightBit_error().raise(LOCAL_INFO,
                  "Error while parsing the UserLogLikes settings: The parameter name '" 
                  + input_par_name + "' appears multiple times in the input section for "
                  "the loglike '" + loglike_name + "'."
                );
              }

              // Check that all parameter names listed under "input" in the "UserLogLikes" 
              // section are also specified in the "UserModel" section.
              // if (not userModelNode[input_par_name].IsDefined())
              bool in_listed_user_pars = std::find(listed_user_pars.begin(), listed_user_pars.end(), input_par_name) != listed_user_pars.end();
              bool in_listed_model_pars = std::find(listed_model_pars.begin(), listed_model_pars.end(), input_par_name) != listed_model_pars.end();
              if(not in_listed_user_pars)
              {
                if(in_listed_model_pars)
                {
                  LightBit_error().raise(LOCAL_INFO,
                    "Error while parsing the UserLogLikes settings: The parameter '" 
                    + input_par_name + "' is requested as an input parameter for the "
                    "loglike '" + loglike_name + "', but this parameter has been assigned "
                    "a new name in the UserModel section. Use this new name in the 'input' "
                    "section for '" + loglike_name + "', or remove the 'name' option in "
                    "the UserModel section."
                  );
                }
                else
                {
                  LightBit_error().raise(LOCAL_INFO,
                    "Error while parsing the UserLogLikes settings: The parameter '" 
                    + input_par_name + "' is requested as an input parameter for the "
                    "loglike '" + loglike_name + "', but it is not found in the "
                    "UserModel section."
                  );
                }
              }

              // Register requested input parameter
              inputs.push_back(input_par_name);
            }
          }
          else  // The current "UserLogLike" entry has no "input" node
          {
            // If there is no "input" node we assume that all listed 
            // parameters should be used as input.
            inputs.assign(listed_user_pars.begin(), listed_user_pars.end());
          }

          if (userLogLikesEntry["output"].IsDefined())
          {
            const YAML::Node output_node = userLogLikesEntry["output"];
            size2 = output_node.size();
            for (std::size_t j = 0; j < size2; j++)
            {
              std::string output_name = output_node[j].as<std::string>();
              // Throw error if there already exists an output with the same output_name
              if (std::find(all_outputs.begin(), all_outputs.end(), output_name) != all_outputs.end())
              {
                LightBit_error().raise(LOCAL_INFO, 
                  "Error while parsing the UserLogLikes settings: multiple outputs "
                  "with the same name '" + output_name + "'. Each output must be " 
                  "assigned a unique name."
                );
              }
              outputs.push_back(output_name);
              all_outputs.push_back(output_name);
            }
          }

          logger() << "Configuration for the loglike '" << loglike_name << "':" << endl;
          logger() << "  user_lib: " << user_lib << endl;
          logger() << "  func_name: " << func_name << endl;
          logger() << "  lang:     " << lang << EOM;

          if (lang == "c" or lang == "c++" or lang == "fortran")
          {
            try
            {
              Gambit::gambit_light_interface::init_user_lib_C_CXX_Fortran(user_lib, func_name, lang, loglike_name, outputs);
            }
            catch (const std::runtime_error& e)
            {
              LightBit_error().raise(LOCAL_INFO, 
                "Caught runtime error while initialising the "
                "gambit_light_interface: " + std::string(e.what())
              );
            }
          }

          #ifdef HAVE_PYBIND11
            if (lang == "python")
            {
              try
              {
                Gambit::gambit_light_interface::init_user_lib_Python(user_lib, func_name, loglike_name, outputs);
              }
              catch (const std::runtime_error& e)
              {
                LightBit_error().raise(LOCAL_INFO,
                  "Caught runtime error while initialising the "
                  "gambit_light_interface: " + std::string(e.what())
               );
              }
            }
          #endif

          // Add loglike_name to list of loglikes
          user_loglikes.push_back(loglike_name);

          ++it1;
        }

        initialisation_done = true;
      }  // End initialisation 

      result = initialisation_done;
    }


    // This function constructs the parameter_point instance needed by 
    // the 'output' function which calls the gambit_light_interface library.
    void input_point(parameter_point& result)
    {
      using namespace Pipes::input_point;

      // Only the first time this function is run: 
      // Construct static vectors with parameter names and 
      // pointers to the parameter values in Param.
      static std::vector<std::string> input_names;
      static std::vector<const double*> input_val_ptrs;
      static bool first = true;

      if (first)
      {
        // For each parameter, add a pointer input_val_ptrs
        // to the corresponding parameter value in Param
        for (const std::string& user_par_name: listed_user_pars)
        {
          const std::string& model_par_name = user_to_model_par_names[user_par_name];
          if (Param.count(model_par_name) == 0) 
          {
            LightBit_error().raise(LOCAL_INFO, 
              "The parameter '" + model_par_name + "' is not a valid UserModel parameter. "
              "Valid parameters are 'p0', 'p1', 'p2', ..."
            );
          }
          input_names.push_back(user_par_name);
          input_val_ptrs.push_back(Param[model_par_name].operator->());
        }

        first = false;
      }

      // Every time this function is run: Fill the result variable (parameter_point instance)
      result.clear();
      for (std::size_t i=0; i < input_names.size(); ++i)
      {
        result.append(input_names[i], *input_val_ptrs[i]);
      }
    }


    void input(map_str_dbl& result)
    {
      using namespace Pipes::input;
      const parameter_point& input_pt = *Dep::input_point;
      result = input_pt.get_map();
    }



    // This function will run the gambit_light_interface library 
    // and collect all the results from all the connected user libraries
    // in one map<string,double>.
    void output(std::map<std::string,double>& result)
    {
      using namespace Pipes::output;

      double total_loglike = 0.0;

      // const std::map<std::string,double>& input = *Dep::input;
      const parameter_point& input_pt = *Dep::input_point;

      // TODO: Make input specific to each loglike function
      std::vector<std::string> input_names = input_pt.get_names();
      std::vector<double> input_vals = input_pt.get_vals();

      // Loop over registered user loglikes
      for (const std::string& loglike_name : user_loglikes)
      {

        double loglike = 0.0;
        std::map<std::string,double> output;
        std::vector<std::string> warnings;

        // Call run_user_loglike from the interface library. 
        // This will fill the result map (and the 'warnings' vector).
        try
        {
          loglike = Gambit::gambit_light_interface::run_user_loglike(loglike_name, input_names, input_vals, output, warnings);
        }
        catch (const std::runtime_error& e)
        {
          std::string errmsg(e.what());

          if (errmsg.substr(0,9) == "[invalid]")
          {
            errmsg.erase(0,9);
            invalid_point().raise(errmsg);
          }
          else if (errmsg.substr(0,7) == "[fatal]")
          {
            errmsg.erase(0,7);
            LightBit_error().raise(LOCAL_INFO, errmsg);
          }
          else
          {
            LightBit_error().raise(LOCAL_INFO, "Caught an unrecognized runtime error: " + errmsg);
          }
        }

        // Log any warnings that we have collected.
        for (const std::string& w : warnings) 
        {
          LightBit_warning().raise(LOCAL_INFO, w);
        }

        // Fill result map
        for (const auto& kv : output)
        {
          result[kv.first] = kv.second;
        }

        // Add this loglike contribution to the result map
        result[loglike_name] = loglike;

        // Add to total loglike
        total_loglike += loglike;

      } // End loop over user loglikes

      result["total_loglike"] = total_loglike;
    }


    // This function will extract the expected 'total_loglike' entry
    // from the gambit_light_interface output map.
    void total_loglike(double& result)
    {
      using namespace Pipes::total_loglike;

      // Get the output map from dependency "light_output"
      const std::map<std::string,double> &output = *Dep::output;

      result = output.at("total_loglike");
    }


    /// @}

  }

}

