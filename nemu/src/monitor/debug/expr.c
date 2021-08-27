#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,Number,Hex,Reg,NEQ,AND,OR

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


	for(i=0; i < 32; i++){
		tokens[i].type = 777;
		memset(tokens[i].str, 0, sizeof(tokens[i].str));

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:{ nr_token--;break;}
					case Number: {
						tokens[nr_token].type = Number;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						break;
					}
					case Hex: {
						tokens[nr_token].type = Hex;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						break;
					}{
					case Reg: {
						tokens[nr_token].type = Reg;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						break;
					}
					default:{tokens[nr_token].type = rules[i].token_type;break;}
				}
                nr_token++;
				break;
			}
		}
		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
nr_token = nr_token;
	return true; 
}




	}return 0;}
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

int mo(int p,int q)
{
	int cnt=0;int priority=-22;int pi=0;
		int i;
		for(i=q;q>=p;q--)
	{
       if(tokens[i].type == ')')cnt++;
	   if(tokens[i].type == '(')cnt--;
	   if(cnt==0)
	   {
		   if(pri(q)>priority)
         {priority=pri(q);pi=q;}
	   }
	   return pi;
	}
return 0;
}

int recursion(int p,int q)
{
	if(check_parentheses(p,q)==true){p++;q--;}
	if(p==q)
	{
		if(tokens[p].type==Number)
		printf("%s",tokens[p].str);
		if(tokens[p].type==Hex)
		printf("%s",tokens[p].str);
	}
   if(p<q)
   {
	  if(++p==q)
     {

     }

else
{
   int mainop =mo(p,q);
   int type = tokens[mainop].type;
int val1 =recursion(p,mainop-1);
int val2 =recursion(mainop+1,q);
switch(type)
{
	case '+':return val1+val2;
	case '-':return val1-val2;
	case '*':return val1*val2;
	case '/':return val1/val2;
}
}

   }
   return 0;
}


uint32_t expr(char *e, bool *success) 
{
	if(!make_token(e)) 
		
		{*success = false;
		return 0;
		}
		


	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return recursion(0,nr_token-1);
}       