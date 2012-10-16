#include "condition_language.h"
#include <string.h>
#include <stdint.h>

namespace condition_language {

namespace {
	#define CONSUME(s, c) { Status status = consume((s), (c)); if (SUCCESS != SUCCESS) return status; }
	#define PARSE(what, s, ctx) { Status status = parse_##what((s), (ctx)); if (status != SUCCESS) return status; }
	#define PUSH(s, val) { if (bytes_left((s)) < sizeof((val))) { return STACK_OVERFLOW; } else { push(s, val); } }
	#define POP(s) pop(s)
	#define POP_ID(s) pop_id(s)
	#define EVAL(c) { Status s = evaluate_single(c); if (s != SUCCESS) return s; }

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

	enum Operator { AND, OR, NEGATE };

	struct Context {
		Stack stack;
		Stack operators;
		Intrinsic* intrinsics;
		unsigned num_intrinsics;
		HashFunction hash;
		void* user_data;
		unsigned num_params;

		struct {
			const char* intrinsic;
			unsigned length;
		} error;
	};

	inline unsigned bytes_left(Stack& s) { return max(int(s.end - s.head), 0); }
	inline void push(Stack& s, IdString32 val) { *(IdString32*)s.head = val; s.head += sizeof(IdString32); }
	inline void push(Stack& s, char val) { *s.head = val; s.head += 1; }
	inline unsigned char pop(Stack& s) { return *(s.head -= 1); }
	inline IdString32 pop_id(Stack& s) { return *(IdString32*)(s.head -= sizeof(IdString32)); }

	inline Status evaluate_single(Context& c) {
		char a, b;
		switch (POP(c.operators)) {
			case NEGATE:
				PUSH(c.stack, char(!POP(c.stack)));
				return SUCCESS;
			case AND:
				a = POP(c.stack);
				b = POP(c.stack);
				PUSH(c.stack, char(a & b));
				return SUCCESS;
			case OR:
				a = POP(c.stack);
				b = POP(c.stack);
				PUSH(c.stack, char(a | b));
				return SUCCESS;
		}
		return PARSE_FAILURE;
	}

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

	inline Status parse_identifier(const char*& s, Context& c) {
		static CharacterClass identifier = "abcdefghijklmnopqrstuvxyzwABCDEFGHIJKLMNOPQRSTUVXYZW_0123456789";
		const char* start = s;
		skip(s, identifier);
		PUSH(c.stack, c.hash(start, s - start));
		return SUCCESS;
	}

	inline Status parse_parameters(const char*& s, Context& c);
	inline Status parse_parameters_rest(const char*& s, Context& c) {
		whitespace(s);
		if (s[0] == ',') {
			CONSUME(s, ',');
			PARSE(parameters, s, c);
		}
		return SUCCESS;
	}

	inline Status parse_parameters(const char*& s, Context& c) {
		whitespace(s);
		PARSE(identifier, s, c);
		++c.num_params;
		PARSE(parameters_rest, s, c);
		return SUCCESS;
	}

	inline Status parse_logical(const char*& s, Context& c);
	inline Status parse_intrinsic(const char*& s, Context& c) {
		whitespace(s);

		if (s[0] == '(') {
			CONSUME(s, '(');
			PARSE(logical, s, c);
			CONSUME(s, ')');
			return SUCCESS;
		}

		const char* start = s;
		PARSE(identifier, s, c);
		IdString32 name = POP_ID(c.stack);
		
		// Look for a matching intrinsic function.
		Intrinsic* intrinsic = 0;
		for (unsigned i = 0; i != c.num_intrinsics; ++i) {
			if (c.intrinsics[i].name == name) {
				intrinsic = &c.intrinsics[i];
				break;
			}
		}

		if (!intrinsic) {
			c.error.intrinsic = start;
			c.error.length = s - start;
			return UNDEFINED_INTRINSIC;
		}

		whitespace(s);
		CONSUME(s, '(');
		c.num_params = 0;
		PARSE(parameters, s, c);
		whitespace(s);
		CONSUME(s, ')');

		char result = intrinsic->function((IdString32*)c.stack.head - c.num_params, c.num_params, c.user_data);
		for (unsigned i = 0; i != c.num_params; ++i)
			POP_ID(c.stack);

		PUSH(c.stack, result);
		return SUCCESS;
	}

	inline Status parse_ternary(const char*& s, Context& c) {
		whitespace(s);
		bool eval = false;
		if (s[0] == '!') {
			CONSUME(s, '!');
			PUSH(c.operators, char(NEGATE));
			PARSE(ternary, s, c);
			eval = true;
		} else {
			PARSE(intrinsic, s, c);
		}

		if (eval)
			EVAL(c);
		return SUCCESS;
	}

	inline Status parse_logical(const char*& s, Context& c);
	inline Status parse_logical_rest(const char*& s, Context& c) {
		whitespace(s);
		if (s[0] == '&') {
			CONSUME(s, '&');
			CONSUME(s, '&');
			PUSH(c.operators, char(AND));
		} else if (s[0] == '|') {
			CONSUME(s, '|');
			CONSUME(s, '|');
			PUSH(c.operators, char(OR));
		} else {
			return SUCCESS;
		}

		PARSE(logical, s, c);
		EVAL(c);
		return SUCCESS;
	}

	inline Status parse_logical(const char*& s, Context& c) {
		PARSE(ternary, s, c);
		PARSE(logical_rest, s, c);
		return SUCCESS;
	}

	inline Status parse_condition(const char*& s, Context& c) {
		return parse_logical(s, c);
	}
}

Result run(
	const char* source,
	unsigned char* stack_memory,
	unsigned stack_size,
	Intrinsic* intrinsics,
	unsigned num_intrinsics,
	HashFunction hash,
	void* user_data)
{
	unsigned char op_stack[64]; // 64 nested operators
	Stack execution_stack = {stack_memory, stack_memory, stack_memory + stack_size};
	Stack operator_stack = {op_stack, op_stack, op_stack + sizeof(op_stack)};
	Context context = {execution_stack, operator_stack, intrinsics, num_intrinsics, hash, user_data};

	Status s = parse_condition(source, context);

	if (s != SUCCESS)
		return Result(s, context.error.intrinsic, context.error.length);

	return Result(POP(context.stack));
}

} // condition_language
