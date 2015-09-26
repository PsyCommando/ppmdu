#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <functional>
#include <memory>

#include <Poco/Util/Application.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/Logger.h>
#include <Poco/SimpleFileChannel.h>
#include <Poco/Path.h>
#include <Poco/File.h>

using namespace std;
using namespace Poco;
using namespace Poco::Util;

const string DEFAULT_LOG = "log.txt";


//
//  Callback Objects
//

//class dfarc_option_callback : public AbstractOptionCallback
//{
//public:
//    typedef void(*fun_t)(AppDFARCTP *, const std::string &, const std::string &);
//
//	void invoke(const std::string& name, const std::string& value) const
//    {
//        _myfun( , name, value);
//        //dynamic_cast<AppDFARCTP*>(&AppDFARCTP::instance())->HandleOptionLog(value);
//    }
//		
//	AbstractOptionCallback* clone() const
//    { 
//        return new dfarc_option_callback(_myfun); 
//    }
//
//    dfarc_option_callback( std::function<fun_t> fun )
//        :_myfun(move(fun))
//    {}
//
//	~dfarc_option_callback()
//    {}
//
//    std::function<fun_t> _myfun;
//};


//
//
//
class AppDFARCTP : public Application
{
public:

    AppDFARCTP()
        :m_logfile(new SimpleFileChannel)
    {
    }

	void initialize(Application& self)
    {
        Application::initialize(self);
    }
		
	void uninitialize()
    {
        Application::uninitialize();
    }

	void reinitialize(Application& self)
    {
        Application::reinitialize(self);
    }

	void defineOptions(Poco::Util::OptionSet& options)
    {
        Application::defineOptions(options);

        options.addOption
            ( 
                Option("log", "l").argument("logpath", false)
                                  .description("Enable logging. If path to log file specified, will log to that file.")
                                  .callback( OptionCallback<AppDFARCTP>( this, &AppDFARCTP::HandleOptionLog ) )
            );
    }

	void handleOption(const std::string& name, const std::string& value)
    {
        Application::handleOption(name, value);
    }

	int main(const std::vector<std::string>& args)
    {
		/// The application's main logic.
		///
		/// Unprocessed command line arguments are passed in args.
		/// Note that all original command line arguments are available
		/// via the properties application.argc and application.argv[<n>].
		///
		/// Returns an exit code which should be one of the values
		/// from the ExitCode enumeration.
        return ExitCode::EXIT_OK;
    }


    void HandleOptionLog( const std::string & name, const std::string & value )
    {
        if( value.empty() )
            m_logfile->setProperty( "path", DEFAULT_LOG );
        else
            m_logfile->setProperty( "path", value );

        Logger::root().setChannel( m_logfile );
    }

private:
    AutoPtr<Poco::SimpleFileChannel> m_logfile;

};

//
//  MAIN
//
POCO_APP_MAIN(AppDFARCTP)