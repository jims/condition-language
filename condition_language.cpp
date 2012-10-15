#include "condition_language.h"
#include <stdint.h>

namespace condition_language {

namespace {
	template<class T>
	inline T max(const T& a, const T& b) {
		return a > b ? a : b;
	}

	struct CharacterClass
	{
		uint64_t data[4];

		CharacterClass() 						{data[0] = data[1] = data[2] = data[3] = 0;}
		CharacterClass(const char *s)			{data[0] = data[1] = data[2] = data[3] = 0; set(s);}

		void set(const char *s) {
			while (*s) {
				unsigned char c = *(const unsigned char *)s;
				data[c/64] = data[c/64] | (1ull << (c%64));
				++s;
			}
		}
		inline uint64_t contains(unsigned char c) const {
			return data[c/64] & (1ull << (c%64));
		}
	};

	inline void skip(const char*& s, const CharacterClass &cc) {
		while (cc.contains(*s))
			++s;
	}

	struct Stack {
		unsigned char* head;
		unsigned char* start;
		unsigned char* end;
	};
	enum { AND, OR, NEGATE };

	struct Context {
		Stack stack;
		Stack operators;
		Intrinsic* intrinsics;
		unsigned num_intrinsics;
		void* user_data;

		struct {
			const char* intrinsic;
			unsigned length;
		} error;
	};

	inline unsigned bytes_left(Stack& s) { return max(s.end - s.head, 0l); }
	inline void push(Stack& s, unsigned char val) { *++s.head = val; }
	inline unsigned char pop(Stack& s) { return *s.head--; }

	inline void whitespace(const char*& s) {
		static CharacterClass whitespace = " \t\n\r";
		skip(s, whitespace);
	}

	inline Status consume(const char*& s, char c) {
		if (*s != c)
			return PARSE_FAILURE;
		++s;
		return SUCCESS;
	}

	inline Status consume(const char*& s, CharacterClass& cc) {
		if (!cc.contains(*s))
			return PARSE_FAILURE;
		++s;
		return SUCCESS;
	}

	#define CONSUME(s, c) { Status status = consume((s), (c)); if (SUCCESS != SUCCESS) return status; }
	#define PARSE(what, s, ctx) { Status status = parse_##what((s), (ctx)); if (status != SUCCESS) return status; }
	#define PUSH(s, val) { if (bytes_left((s)) < sizeof((s))) { return STACK_OVERFLOW } else { push(s, val) } }
	#define POP(s) pop(s)

	inline Status parse_condition(const char*& s, Context& c) {
		return parse_logical(s, c);
	}

	inline Status parse_logical(const char*& s, Context& c) {
		PARSE(ternary, s, c);
		whitespace(s);
		char op = s[0];

		if (op == '&') {
			CONSUME(s, '&');
			CONSUME(s, '&');
			PUSH(c.operators, AND);
		} else if (op == '|') {
			CONSUME(s, '|');
			CONSUME(s, '|');
			PUSH(c.operators, OR);
		}
		PARSE(ternary, s, c);
		char a = POP(c.stack), b = POP(c.stack);

	}

	inline Status parse_ternary(const char*& s, Context& c) {

	}

	inline Status parse_intrinsic(const char*& s, Context& c) {
		static CharacterClass identifier = "abcdefghijklmnopqrstuvxyzwABCDEFGHIJKLMNOPQRSTUVXYZW_0123456789";
		
		whitespace(s);
		const char* start = s;
		skip(s, identifier);
		unsigned len = s - start;
		CONSUME(s, '(');
	}
}

Result run(
	const char* source,
	unsigned char* stack_memory,
	unsigned stack_size,
	Intrinsic* intrinsics,
	unsigned num_intrinsics,
	void* user_data)
{
	unsigned char op_stack[64] = {0};
	Stack execution_stack = {stack_memory, stack_memory, stack_memory + stack_size};
	Stack operator_stack = {op_stack, op_stack, op_stack + sizeof(op_stack)};
	Context context = {execution_stack, operator_stack, intrinsics, num_intrinsics, user_data, 0, 0};

	Status s = parse_condition(source, context);

	if (s != SUCCESS)
		return Result(s, context.error.intrinsic, context.error.length);

	return Result(pop(execution_stack));
}

} // condition_language
