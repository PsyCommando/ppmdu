#include "ppmd3_txtutil.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdint>

using namespace std;
using namespace Poco::Util;

namespace txt_util
{
//=========================================================================================
//  Overrides
//=========================================================================================
	void AppTxtUtil::initialize(Application& self)
    {
        Application::initialize(self);
		/// Initializes the application and all registered subsystems.
		/// Subsystems are always initialized in the exact same order
		/// in which they have been registered.
		///
		/// Overriding implementations must call the base class implementation.
    }
		
	void AppTxtUtil::uninitialize()
    {
        Application::uninitialize();
		/// Uninitializes the application and all registered subsystems.
		/// Subsystems are always uninitialized in reverse order in which
		/// they have been initialized. 
		///
		/// Overriding implementations must call the base class implementation.
    }

	void AppTxtUtil::reinitialize(Application& self)
    {
        Application::reinitialize(self);
		/// Re-nitializes the application and all registered subsystems.
		/// Subsystems are always reinitialized in the exact same order
		/// in which they have been registered.
		///
		/// Overriding implementations must call the base class implementation.
    }

	void AppTxtUtil::defineOptions(OptionSet& options)
    {
        Application::defineOptions(options);
		/// Called before command line processing begins.
		/// If a subclass wants to support command line arguments,
		/// it must override this method.
		/// The default implementation does not define any options itself,
		/// but calls defineOptions() on all registered subsystems.
        ///
		/// Overriding implementations should call the base class implementation.
    }


	void AppTxtUtil::handleOption(const std::string& name, const std::string& value)
    {
        Application::handleOption( name, value );
		/// Called when the option with the given name is encountered
		/// during command line arguments processing.
		///
		/// The default implementation does option validation, bindings
		/// and callback handling.
		///
		/// Overriding implementations must call the base class implementation.
    }

	int AppTxtUtil::main(const std::vector<std::string>& args)
    {
		/// The application's main logic.
		///
		/// Unprocessed command line arguments are passed in args.
		/// Note that all original command line arguments are available
		/// via the properties application.argc and application.argv[<n>].
		///
		/// Returns an exit code which should be one of the values
		/// from the ExitCode enumeration.
        cout<<"HeyHo!\n";
        
        this_thread::sleep_for( chrono::seconds(5) );

        return EXIT_OK;
    }

//=========================================================================================
//  Options
//=========================================================================================


//=========================================================================================
//  Processing
//=========================================================================================

};

//
//  MAIN
//
POCO_APP_MAIN(txt_util::AppTxtUtil)