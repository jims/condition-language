#pragma once

typedef unsigned IdString32;

/*
	Grammar
	---------------------------------
	CONDITION :=
		LOGICAL
	
	LOGICAL :=
		TERNARY '&&' TERNARY
		| TERNARY '||' TERNARY
		| TERNARY

	TERNARY :=
		'!' INTRINSIC
		| INTRINSIC

	INTRINSIC :=
		IDENTIFIER '(' PARAMETERS ')'
		| '(' CONDITION ')'
*/

namespace condition_language {
	typedef bool(*IntrinsicFunction)(IdString32* arguments, unsigned n_arguments, void* user_data);

	struct Intrinsic {
		IdString32 name;
		IntrinsicFunction function;
	};

	enum Status {
		SUCCESS,			///< Execution completed successfully. The resulting value is stored in the value-member.
		STACK_OVERFLOW,		///< Out of stack memory during execution.
		PARSE_FAILURE,		///< There was a a problem parsing the expression.
		UNDEFINED_INTRINSIC ///< A call to an unknown instrinsic was made. The missing instrinsic name in stored in the intrinsic-member.
	};

	struct Result {
		Status exit_status;
		union {
			bool value;
			struct {
				const char* missing_intrinsic;
				unsigned length;
			} error_info;
		};
		Result(Status s) : exit_status(s) { }
		Result(bool result) : exit_status(SUCCESS), value(result) { }
		Result(Status s, const char* intrin, unsigned length) : exit_status(s) { error_info.missing_intrinsic = intrin; error_info.length = length; }
	};

	Result run(
			const char* source,											///< Input expression
			unsigned char* stack_memory,	unsigned stack_size,		///< Some memory for the execution stack
			Intrinsic* intrinsics,			unsigned num_intrinsics,	///< A list of intrinsic function callbacks
			void* user_data												///< User data parameter passed to intrinsic callbacks
		);
}
