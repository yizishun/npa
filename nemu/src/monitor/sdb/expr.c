/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#define MAXOP 10
enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMD,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-",'-'},            // sub
  {"\\*",'*'},          // mul
  {"/",'/'},            // div
  {"\\(",'('},          // lp
  {"\\)",')'},          // rp
  {"[0-9]{1,}",TK_NUMD},   //number dec  
  {"==", TK_EQ},        // equal
  
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
	
	
        switch (rules[i].token_type) {
	  case '+':case'-':case '*':case '/':case '(':case ')':
	    tokens[nr_token++].type = rules[i].token_type;
	    break;
	  case TK_NOTYPE:break;
	  case TK_NUMD:
	    tokens[nr_token++].type = rules[i].token_type;
	    strncpy(tokens[nr_token-1].str,substr_start,substr_len);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}
bool check_parentheses2(int p,int q){
  if(p > q) assert(0);
  int lp = 0;
  for(;p < q;p++){
    if(tokens[p].type == '(')
      lp += 1;
    else if(tokens[p].type == ')'){
      if(lp == 0) return false;
      else lp -= 1;
    }
}
  if(lp != 0) return false;
  else return true;



}
bool check_parentheses(int p,int q){

  if(check_parentheses2(p , q) == false)
    assert(0);
  if((tokens[p].type == '(')&&(tokens[q].type == ')')){
    p+=1;
    q-=1;
    return check_parentheses2(p , q);}
  return false;
}

static int find_main_op(int p,int q){
  int plus[MAXOP] = {-1}, plusptr = 0;
  int sub[MAXOP] = {-1},subptr = 0;
  int mul[MAXOP] = {-1}, mulptr = 0;
  int div[MAXOP] ={-1}, divptr = 0;
  int lp = 0;
  int op = 0;
  for(;p < q;p++){
    if(tokens[p].type == '(') lp++;
    if(tokens[p].type == ')') lp--;
    if(lp != 0) continue;
    switch(tokens[p].type){
      case '+' : 
        plus[plusptr++] = p;
	break;
      case '-' :
	sub[subptr++] = p;
	break;
      case '*' :
	mul[mulptr++] = p;
	break;
      case '/' :
	div[divptr++] = p;
	break;
      default : continue;
}}
    if(plus[0] != -1) op = plus[--plusptr];
    if(sub[0] != -1)
      if(sub[--subptr] > op) op = sub[subptr];
    if((plus[0] == -1) &&(sub[0] == -1)){
      if(mul[0] != -1) op = mul[--mulptr];
      if(div[0] != -1)
        if(div[--divptr] > op) op = div[divptr];
    }
  return op;
} 

static uint32_t eval(int p,int q){
  int op;
  int val1,val2;
  if(p > q)
    assert(0);
  else if(p == q)
    return atoi(tokens[p].str);
  else if(check_parentheses(p,q) == true)
    return eval(p + 1, q - 1);
  else{
    op = find_main_op(p,q);
    val1 = eval(p , op -1);
    val2 = eval(op + 1 ,q);
    switch(tokens[op].type){
      case '+':return val1 + val2;
      case '-':return val1 - val2;
      case '*':return val1 * val2;
      case '/':return val1 / val2;
      default :assert(0);
    }
  }



}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  return eval(0,nr_token-1);
}















