#ifndef PPMD3_TXTUTIL_HPP
#define PPMD3_TXTUTIL_HPP
/*
ppmd3_txtutil.hpp
2015/08/28
psycommando@gmail.com
Description: Utility for reading and writing text strings into PMD3 games.
*/
#include <Poco/Util/Application.h>
#include <string>

namespace txt_util
{

//=========================================================================================
//  AppTxtUtil
//=========================================================================================
    class AppTxtUtil : public Poco::Util::Application
    {
    public:
	void initialize(Application& self);
		/// Initializes the application and all registered subsystems.
		/// Subsystems are always initialized in the exact same order
		/// in which they have been registered.
		///
		/// Overriding implementations must call the base class implementation.
		
	void uninitialize();
		/// Uninitializes the application and all registered subsystems.
		/// Subsystems are always uninitialized in reverse order in which
		/// they have been initialized. 
		///
		/// Overriding implementations must call the base class implementation.

	void reinitialize(Application& self);
		/// Re-nitializes the application and all registered subsystems.
		/// Subsystems are always reinitialized in the exact same order
		/// in which they have been registered.
		///
		/// Overriding implementations must call the base class implementation.

	void defineOptions(Poco::Util::OptionSet& options);
		/// Called before command line processing begins.
		/// If a subclass wants to support command line arguments,
		/// it must override this method.
		/// The default implementation does not define any options itself,
		/// but calls defineOptions() on all registered subsystems.
		///
		/// Overriding implementations should call the base class implementation.

	void handleOption(const std::string& name, const std::string& value);
		/// Called when the option with the given name is encountered
		/// during command line arguments processing.
		///
		/// The default implementation does option validation, bindings
		/// and callback handling.
		///
		/// Overriding implementations must call the base class implementation.

	int main(const std::vector<std::string>& args);
		/// The application's main logic.
		///
		/// Unprocessed command line arguments are passed in args.
		/// Note that all original command line arguments are available
		/// via the properties application.argc and application.argv[<n>].
		///
		/// Returns an exit code which should be one of the values
		/// from the ExitCode enumeration.

    private:
    };
};

#endif