#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,Number,Hex,Reg,NEQ,AND,OR,NEG,DEREF

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"==", EQ}	,					// equal
	{"0[xX][A-Fa-f0-9]{1,8}", Hex},	//16进制
	{"[0-9]{1,10}", Number},		//数字
	{"\\-", '-'},					// 减
	{"\\*", '*'},					// 乘
	{"/", '/'},						// 除
	{"\\(", '('},					//	( 
	{"\\)", ')'},					//	)
	{"!=", NEQ},					//不等
	{"&&", AND},						//逻辑与
	{"\\|\\|", OR},					//逻辑或
	{"!", '!'}		,				//逻辑非
	{"\\$(e?(ax|dx|cx|bx|si|di|sp|ip)|[a-d][hl])", Reg},	//寄存器
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them onAND once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE: break;
					case Number:
					//case ID:
					case Reg: sprintf(tokens[nr_token].str, "%.*s", substr_len, substr_start);
					default: tokens[nr_token].type = rules[i].token_type;
							 nr_token ++;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

	bool check_parentheses(int p ,int q)
{
    int i,valid = 0;
    if(tokens[p].type != '(' || tokens[q].type != ')') return false; 
    for(i = p ; i <= q ; i ++){    
        if(tokens[i].type == '(') valid++;
        else if(tokens[i].type == ')') valid--;
        if(valid == 0 && i < q) return false ; 
    }                              
    if( valid != 0 ) return false;   
    return true;                   
}

int pri(int a)
{
	switch(a)
	{
		case '+' :return 5;
		case '-'	:return 5;
		case AND:return 12;
		case OR:return 13;
		case '*':return 4;
		case '/':return 4;
		case '!':return 2;
		}
		return -1;
}

int mo(int p, int q)
{

	int i, dom = p, left_n = 0;
	int pr = -1;
	for (i = p; i <= q; i++)
	{
		if (tokens[i].type == '(')
		{
			left_n += 1;
			i++;
			while (1)
			{
				if (tokens[i].type == '(')
					left_n += 1;
				else if (tokens[i].type == ')')
					left_n--;
				i++;
				if (left_n == 0)
					break;
			}
			if (i > q)
				break;
		}
		else if (tokens[i].type == Number)
			continue;
		else if (pri(tokens[i].type) > pr)
		{
			pr = pri(tokens[i].type);
			dom = i;
		}
	}

	return dom;
}

uint32_t eval(int l, int r, bool *success)
{
	if (*success == 0)
		return -1;
	if (l > r)
	{
		Assert(l > r, "表达式计算错误未知!\n");
		return -1;
	}
	if (l == r)
	{
		uint32_t num = 0;
		if (tokens[l].type == Number)
			sscanf(tokens[l].str, "%d", &num);
		else if (tokens[l].type == Hex)
			sscanf(tokens[l].str, "%x", &num);
		else if (tokens[l].type == Reg)
		{
			if (strlen(tokens[l].str) == 3)
			{
				int i;
				for (i = R_EAX; i <= R_EDI; i++)
					if (strcmp(tokens[l].str, regsl[i]) == 0)
						break;
				if (i > R_EDI)
					if (strcmp(tokens[l].str, "eip") == 0)
						num = cpu.eip;
					else
						Assert(1, "no this register!\n");
				else
					num = reg_l(i);
			}
			else if (strlen(tokens[l].str) == 2)
			{
				if (tokens[l].str[1] == 'x' || tokens[l].str[1] == 'p' || tokens[l].str[1] == 'i')
				{
					int i;
					for (i = R_AX; i <= R_DI; i++)
						if (strcmp(tokens[l].str, regsw[i]) == 0)
							break;
					num = reg_w(i);
				}
				else if (tokens[l].str[1] == 'l' || tokens[l].str[1] == 'h')
				{
					int i;
					for (i = R_AL; i <= R_BH; i++)
						if (strcmp(tokens[l].str, regsb[i]) == 0)
							break;
					num = reg_b(i);
				}
				else
					assert(1);
			}
		}

		else
		{
			//printf("type = %d\n", token[l].type);
			*success = false;
			return -1;
		}
		return num;
	}
	else if (check_parentheses(l, r) == true)
		return eval(l + 1, r - 1, success);
	else
	{
		int op = mo(l, r);
		if (l == op || tokens[op].type == DEREF || tokens[op].type == NEG || tokens[op].type == '!')
		{
			uint32_t val = eval(l + 1, r, success);
			switch (tokens[l].type)
			{
			case DEREF:
				return swaddr_read(val, 4);
			case NEG:
				return -val;
			case '!':
				return !val;
			default:
				//printf("不合法表达式\n"); //有些主运算符无法处于第一位
				*success = false;
				return -1;
			}
		}
		uint32_t val1 = eval(l, op - 1, success);
		uint32_t val2 = eval(op + 1, r, success);
		switch (tokens[op].type)
		{
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		case EQ:
			return val1 == val2;
		case NEQ:
			return val1 != val2;
		case AND:
			return val1 && val2;
		case OR:
			return val1 || val2;
		default:
			//printf("不合法表达式\n"); //有些主运算符无法处于第一位
			*success = false;
			return -1;
		}
	}
}


uint32_t expr(char *e, bool *success) 
{
	if(!make_token(e)) 
		
		{*success = false;
		return 0;
		}
		
    


	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return eval(0,nr_token-1,success);
}       