#ifndef CMD_LINE_UTIL_RUNNER_HPP
#define CMD_LINE_UTIL_RUNNER_HPP
#include <utils/cmdline_util.hpp>
//#include <iostream>

/*
    Macro to define a main method for a commandline utility
*/
#ifndef CMDLINE_UTILITY_MAIN
    #define CMDLINE_UTILITY_MAIN(UTILTY) int main( int argc, const char * argv[] ) {return UTILTY##::GetInstance().Main(argc,argv);}
#endif
#if 0
namespace utils{ namespace cmdl 
{

    /*
        BaseUtilTypeWrapper
            Interface for accessing the utility pointer regardless of the type of utility.
    */
    class BaseUtilTypeWrapper
    {
    public:
        virtual ~BaseUtilTypeWrapper(){}
        virtual CommandLineUtility * GetInstancePtr()=0;
    };

    /*
        UtilTypeWrapper
            Child class of BaseUtilTypeWrapper, meant to be instantiated with the type of the utility, and passed to the 
            InstanceRunner construtor, to automatically launch the correct utility!
    */
    template<class _TUtil>
        class UtilTypeWrapper : public BaseUtilTypeWrapper
    {
    public:
        CommandLineUtility * GetInstancePtr()
        {
            return &(_TUtil::GetInstance());
        }
    };

    /*
        InstanceRunner
            Create an instance to set the instance, with the instance pointer of a command line utility to run.
    */
    class InstanceRunner
    {
    public:
        //InstanceRunner(CommandLineUtility * inst) 
        //{
        //    s_pinstance = inst;
        //}
        InstanceRunner(BaseUtilTypeWrapper * pwrap) 
        {
            s_wrap = pwrap;
        }

        static inline int RunInstance(int argc, const char * argv[])
        {
            try { return s_wrap->GetInstancePtr()->Main(argc,argv); }
            catch( const std::exception & e )
            {
                std::cerr<< "<!>-ERROR:" <<e.what()<<"\n"
                         << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
                return EXIT_FAILURE;
            }
        }

    private:
        //static CommandLineUtility  * s_pinstance;
        static BaseUtilTypeWrapper * s_wrap;
    };

};};
#endif
//=================================================================================================
// Main Function
//=================================================================================================
//int main( int argc, const char * argv[] ) {return utils::cmdl::InstanceRunner::RunInstance(argc,argv);}

#endif
