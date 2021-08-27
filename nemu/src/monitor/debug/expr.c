#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,Number,Hex,Reg,NEQ,AND,OR,NEG

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

uint32_t eval(int p,int q,bool *success)
{
	if(*success == 0) return 0;
	if(p > q)
	{
		*success = false;
		return 0;
	}
	else if(p == q)
	{
		int a;
		if(tokens[p].type ==Number)
		{
			sscanf(tokens[p].str,"%d",&a);
			*success = true;
			return a;
		}
		if(tokens[p].type == Hex)
		{
			sscanf(tokens[p].str,"%x",&a);
			*success = true;
			return a;
		}

	}
	else if(check_parentheses(p,q) == true)
	{
		return eval(p +1,q - 1,success);
	}
	else
	{
		if((q - p) == 1)
		{
			if(tokens[p].type == NEQ)
			     return !eval(p + 1,q,success);
			if(tokens[p].type == NEG)
			     return 0 - eval(p + 1,q,success);
		}
	}

	int op = mo(p,q);
	int value1 = eval(p,op - 1,success);
	int value2 = eval(op + 1,q,success);
	int op_type = tokens[op].type;
	
	switch(op_type)
	{
		case '+':             return value1 + value2; break;
		case '-':          return value1 - value2; break;
		case '*':            return value1 * value2; break;
		case '/':                 return value1 / value2; break;
		case AND:               return value1 && value2; break;
		case OR:                  return value1 || value2; break;
		case EQ:                  return value1 == value2; break;
		case NEQ:           return value1 != value2; break;
		default:   assert(0);  return 0;
	}
}

uint32_t expr(char *e, bool *success) 
{
	if(!make_token(e)) 
		
		{*success = false;
		return 0;
		}
		
    printf("%d",nr_token);


	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return eval(0,nr_token-1,success);
}       