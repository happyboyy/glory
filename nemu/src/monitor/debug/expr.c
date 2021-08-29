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
 * TheDEREFore we compile them onAND once before any usage.
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
		
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				

				switch(rules[i].token_type) {
                                        case Number : tokens[nr_token].type=rules[i].token_type;
                                        if(substr_len>=32) 
                                          printf("NUM_10 too large");
                                        else{
                                             int j;
                                             for (j = 0;j < 32;j ++)
                                             tokens[nr_token].str[j] = '\0';
                                             for (j =0;j <substr_len;j ++){
                                                 tokens[nr_token].str[j] = substr_start[j];
                                              }
                                             }
                                        break;
                                        case Hex: tokens[nr_token].type = rules[i].token_type;
                                        if (substr_len >=34)
                                             printf("NUM_16 too largee");
                                        else{
                                             int j;
                                             for( j = 0;j < 32;j ++)
                                                tokens[nr_token].str[j] ='\0';
                                             for( j = 0; j < substr_len -2;j ++){
                                                tokens[nr_token].str[j] = substr_start[j + 2];
                                               }
                                            }
                                        break;
                                        case Reg:tokens[nr_token].type = rules[i].token_type;
                                            int j;
                                            for( j = 0;j < 32;j ++)
                                            tokens[nr_token].str[j] = '\0';
                                            for( j = 0; j < substr_len -1;j++)
                                            tokens[nr_token].str[j] = substr_start[j + 1];
                                            break;
                                        case NOTYPE: nr_token--;break;

                                             
					default:tokens[nr_token].type = rules[i].token_type;
                                     // panic("please implement me");
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

static int op_prec(int t) {
	switch(t) {
		case '!': case NEG: case DEREF: return 0;
		case '*': case '/': case '%': return 1;
		case '+': case '-': return 2;
		case EQ: case NEQ: return 4;
		case AND: return 8;
		case OR: return 9;
		default: assert(0);
	}
}


static inline int op_prec_cmp(int t1, int t2) {
	return op_prec(t1) - op_prec(t2);
}

static int find_dominated_op(int s, int e, bool *success) {
	int i;
	int bracket_level = 0;
	int dominated_op = -1;
	for(i = s; i <= e; i ++) {
		switch(tokens[i].type) {
			case Reg: case Number:  break;

			case '(': 
				bracket_level ++; 
				break;

			case ')': 
				bracket_level --; 
				if(bracket_level < 0) {
					*success = false;
					return 0;
				}
				break;

			default:
				if(bracket_level == 0) {
					if(dominated_op == -1 || 
							op_prec_cmp(tokens[dominated_op].type, tokens[i].type) < 0 ||
							(op_prec_cmp(tokens[dominated_op].type, tokens[i].type) == 0 && 
							 tokens[i].type != '!' && tokens[i].type != '~' &&
							 tokens[i].type != NEG && tokens[i].type != DEREF) ) {
						dominated_op = i;
					}
				}
				break;
		}
	}

	*success = (dominated_op != -1);
	return dominated_op;
}


uint32_t eval(int p, int q, bool *success){
	if(*success == 0) return 0;
	if(p > q){
		*success = false;
		return 0;
	}
	else if(p == q){
		int n;
		if(tokens[p].type == Number){
			sscanf(tokens[p].str,"%d",&n);
			*success = true;
			return n;
		}
		if(tokens[p].type == Hex){
			sscanf(tokens[p].str,"%x",&n);
			*success = true;
			return n;
		}
		if(tokens[p].type == Reg){
			int i; *success = true;
			const char* reg_32[8] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
			const char* reg_16[8] = {"ax","cx","dx","bx","sp","bp","si","di"};
			const char* reg_8[8] = {"al","ah","cl","ch","dl","dh","bl","bh"};
			for(i = 0;i < 8;i++){
			   if(strcmp(tokens[p].str,reg_32[i]) == 0){ n = cpu.gpr[i]._32; break;}
			   if(strcmp(tokens[p].str,reg_16[i]) == 0){ n = cpu.gpr[i]._16; break;}
			   if(strcmp(tokens[p].str,reg_8[i]) == 0){ n = cpu.gpr[i/2]._8[i%2]; break;}
			}
			if(strcmp(tokens[p].str,"eip") == 0)
			n = cpu.eip;
			return n;
		}
		else{
			*success = false;
			return 0;
		}
	}
	else if(check_parentheses(p,q) == true){
		return eval(p + 1,q - 1,success);
	}
	else{
		
		/*if((q - p) == 1){
			if(tokens[p].type == NEG) return 0-eval(p + 1,q,success);
			if(tokens[p].type == '!') return !eval(p + 1,q,success);
			if(tokens[p].type == DEREF) 
				return swaddr_read(eval(p + 1,q,success),4);
			else{
				*success = false;
				return 0;
			}
		}*/
		int op = find_dominated_op(p,q,success);
		int op_type = tokens[op].type;
		if(op_type == '!' || op_type == NEG || op_type == DEREF) {
			uint32_t val = eval(op + 1, q, success);
			if(!*success) { return 0; }

		switch(op_type){
			case '!': return !val;
				case NEG: return -val;
				case DEREF: return swaddr_read(val, 4);
			default: assert(0); return 0;
		}
		}
	
		int value1 = eval(p,op - 1,success);
		int value2 = eval(op + 1,q,success);
		
		switch(op_type){
			case '+' : return value1 + value2; break;
			case '-' : return value1 - value2; break;
			case '*': return value1*value2; break;
			case '/': return value1/value2; break;
			case AND : return value1 && value2; break;
			case OR: return value1 || value2; break;
			case NEQ: return value1 != value2; break;
			case EQ: return value1 == value2; break;
			default: assert(0); return 0;
		}
	   }
}
		
uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}


	int i;
	for(i = 0;i < nr_token; i++){
		if((tokens[i].type == '*' || tokens[i].type == '-') && (i == 0 || 
tokens[i - 1].type =='+' || tokens[i - 1].type == '-' || tokens[i - 1].type == '*' ||
 tokens[i - 1].type == '/' || tokens[i - 1].type == '(')){
			if(tokens[i].type == '*') tokens[i].type = DEREF;
			if(tokens[i].type == '-') tokens[i].type = NEG;
	}
	}	
	uint32_t result = 0;
	result = eval(0,nr_token - 1,success);
	//panic("please implement me");
	return result;
}
