// convert c++ exceptions to language native exceptions
%exception {
    try {
        $action
    }
    catch (Swig::DirectorException &e) { 
        SWIG_fail;
    }
    catch (const std::exception& e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

// connect operator<< to tp_repr
%ignore ::operator<<;
%feature("python:slot", "tp_repr", functype="reprfunc") *::__repr__;

%define ADD_REPR(classname)
%extend Ogre::classname {
    const std::string __repr__() {
        std::ostringstream out;
        out << *$self;
        return out.str();
    }
}
%enddef

// connect operator[] to __getitem__
%feature("python:slot", "sq_item", functype="ssizeargfunc") *::operator[];
%rename(__getitem__) *::operator[];

%feature("flatnested");
%feature("autodoc", "1");
%feature("director") *Listener;
%feature("director") *::Listener;

// SWIG macros for convert free operator to member function
%define OPERATOR_TO_MEMBER_FUNCTION(returnType, argTypeA, operatorName, argTypeB, className, functionName)
	%ignore operator operatorName (argTypeA, argTypeB);
	%extend Ogre::className {
		returnType functionName(argTypeA x) {
			return *$self operatorName x;
		}
	}
%enddef

%define OPERATOR_TO_MEMBER_FUNCTION_SHORT(argTypeA, operatorName, argTypeB, className, functionName)
	OPERATOR_TO_MEMBER_FUNCTION(className, argTypeA, operatorName, argTypeB, className, functionName)
%enddef

// SWIG macro for convert class operator to member function
%define CLASS_OPERATOR_TO_MEMBER_FUNCTION(returnType, className, operatorName, argType, functionName)
	%ignore Ogre::className::operator operatorName ( argType );
	%extend Ogre::className {
		returnType functionName(argType x) {
			return *$self operatorName x;
		}
	}
%enddef
