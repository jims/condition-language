Condition Language
==================

Condition language is a tiny boolean expression language written in minimal c++.
It's weighs in around roughly 300 lines of code and is basically just a recursive
decent parser with an execution and operator stack. It evaluates expressions during
parsing and does not have any form of intermediate representation, making it perfect
when you have lots of different conditions needing evaluation only once or a couple of times. 

Example usage
-------------
	// Our intrinsic function callback
    bool equal(IdString32* params, unsigned n_params, void* user_data) {
	    return params[0] == params[1];
    }

	condition_language::Intrinsic intrinsics[] = {
		{ your_hash_function("equal", 5), instrinsic::equal },
	};
	const unsigned num_intrinsics = sizeof(intrinsics) / sizeof(*intrinsics);
	const char* source = "equal(A, A) && !!(equal(B, B) || equal(B, C))";

	// A small execuion stack, 10 bytes would suffice for this expression.
	unsigned char stack[16];
	condition_language::Result r = condition_language::run(
		source,                     // Expression source
		stack, sizeof(stack),       // Execution stack memory
		intrinsics, num_intrinsics, // List of intrinsic functions
		your_hash_function,         // Hash function used for hashing intrinsic function arguments,
							        // must be the same as the one used to provice intrinsic funtion names.
		NULL);                      // User data pointer passed to the intrinsic function callbacks.
	// The resulting value is available in r.value

Grammar
-------
	CONDITION :=
		LOGICAL '\0'
	
	LOGICAL :=
		TERNARY LOGICAL_REST

	LOGICAL_REST :=
		'&&' LOGICAL
		'||' LOGICAL
		| ''

	TERNARY :=
		'!' TERNARY
		| INTRINSIC

	INTRINSIC :=
		IDENTIFIER '(' PARAMETERS ')'
		| '(' LOGICAL ')'

	PARAMTERS :=
		IDENTIFIER PARAMETERS_REST
	
	PARAMETERS_REST :=
		',' PARAMETERS
		| ''

License
-------
It's released under the MIT license. Enjoy!

    The MIT License (MIT)
    Copyright (c) 2012 Jim Sagevid
 
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software
    and associated documentation files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge, publish, distribute,
    sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
     is furnished to do so, subject to the following conditions:
 
    The above copyright notice and this permission notice shall be included in all copies or substantial
    portions of the Software.
 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
    LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
