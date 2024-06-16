// libc includes (available in both C and C++)
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
// Implementation includes
#include "slice.h"
//this initalizes the global variables, the stack is implemented by creating two large arrays that stores the PC of each stack and the localIt of each function call.
//the stack pointer is initialized to 0(SP), and everytime a function is added to the call stack, it gets incremented and vice versa.
//global it is done by reserving localIt[0] for the global variation of the it variable
    uint64_t localIt[10000];
    char * PCtoBranch[10000];
    int stackPointer = 0;
    SliceToIntHashMap* s_table;
    bool consume(const char*);
    uint64_t expression(bool);
    char const * program;
    char const * current;
    uint64_t returnRegister;
    bool statement(bool);    
  void unused(uint64_t bruh)
    {
        bruh = bruh;
        return;
    }
  void fail() {
    printf("failed at offset %ld\n",(size_t)(current-program));
    printf("%s\n",current);
    exit(1);
  }

  void end_or_fail() {
    while (isspace(*current)) {
      current += 1;
    }
    if (*current != 0) fail();
  }

  void consume_or_fail(const char* str) {
    if (!consume(str)) {
      fail();
    }
  }

  void skip() {
      while (isspace(*current)) {
          current += 1;
      }
  }
  //this peeks at the next string, and returning true is the str that we want is peeked
  bool peek(const char* str) {
    char * temp = (char*)current;
    while(isspace(*temp)) {
        temp += 1;
    }
    size_t i = 0;
    while (true) {
      char const expected = str[i];
      char const found = temp[i];
      if (expected == 0) {
        /* survived to the end of the expected string */
            return true;  
        }
      if (expected != found) {
            return false;
        }
      // assertion: found != 0
      i += 1;
    }
  }
  bool consume(const char* str) {
    skip();
    //printf("%s\n",str);
    size_t i = 0;
    while (true) {
      char const expected = str[i];
      char const found = current[i];
      if (expected == 0) {
        /* survived to the end of the expected string */
        current += i;
        return true;
      }
      if (expected != found) {
        return false;
      }
      // assertion: found != 0
      i += 1;
    }
    
  }

  Slice* consume_identifier() {
    skip();

    if (isalpha(*current)) {
      char * start = (char*) current;
      do {
        current += 1;
      } while(isalnum(*current));
      Slice* ptr = (Slice*) malloc(sizeof(Slice));
      ptr->start=start;
      ptr->len=(size_t)(current-start);
      return ptr;
    } else {
      return NULL;
    }
  }

  uint64_t* consume_literal() {
    skip();
    if (isdigit(*current)) {
      uint64_t v = 0;
      do {
        v = 10*v + ((*current) - '0');
        current += 1;
      } while (isdigit(*current));
      uint64_t* ptr = (uint64_t*) malloc(sizeof(uint64_t));
      *ptr = v;
      return ptr;
    } else {
      return NULL;
    }
  }

    // The plan is to honor as many C operators as possible with
    // the same precedence and associativity
    // e<n> implements operators with precedence 'n' (smaller is higher)

    // () [] . -> ...
    
    //the logic here is that if effects is true, then we would actually evalute the logic. If it is false, we would just increment the PC until effects is true.

    uint64_t e1(bool effects) {
        //printf("%s\n","test");
        //the logic here checks for the case where IT is called.
        //honored by checking the stackPointer and then returning it from the localIt array
        if(consume("it"))
        {
            if(!effects)
            {
                if(peek("("))
                {
                    while(consume("("))
                    {
                        expression(effects);
                        consume(")");
                    }    
                }
                return 0;
            }
            else
            {
                uint64_t v = localIt[stackPointer];
                char * currNumToBranch = (char *)(v + (uint64_t) program);
                uint64_t returnValForFunction = 0;
                if(peek("("))
                {
                    
                    while(consume("("))
                    {
                        uint64_t currExpression = expression(effects);
                        consume(")");
                        stackPointer = stackPointer+1;
                        PCtoBranch[stackPointer] = (char *)current;
                        localIt[stackPointer] = currExpression;
                        current = currNumToBranch;
                        if(!consume("{"))
                        {
                            bool returned = false;
                            if(!statement(effects))
                            {
                                returned = true;
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        else
                        {
                            bool returned = false;
                            while(!consume("}"))
                            {
                                if(!statement(effects))
                                {
                                    returned = true;
                                    break;
                                }
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        returnValForFunction = returnRegister;
                        currNumToBranch = (char *)(returnValForFunction + (uint64_t) program);
                    }
                    return returnValForFunction;
                }
                return localIt[stackPointer];
            }
        }
        //fun is served only a sentinel that would not relate to function calls. The actiosn calls will be done if we detect a parenthesis.
        //fun basically tells us to store the pointer to the content after fun if effects is true.
        //when a parenthesis is peeked and we know it is calling a function, we would jump to the pointer stored in the opaque uint64 variables that is stored for functions.
        if(consume("fun"))
        {
            if(!effects)
            {
                if(!consume("{"))
                {
                    statement(effects);
                    if(peek("("))
                    {
                        while(consume("("))
                        {
                            expression(effects);
                            consume(")");
                        }    
                    }
                }
                else
                {
                    while(!consume("}"))
                    {
                        statement(effects);
                    }
                    if(peek("("))
                    {
                        while(consume("("))
                        {
                            expression(effects);
                            consume(")");
                        }    
                    }
                }
                return 0;
            }
            else
            {
                uint64_t functionPointerInt = (uint64_t) (current-program);
                if(!consume("{"))
                {
                    statement(false);
                }
                else
                {
                    while(!consume("}"))
                    {
                        statement(false);
                    }
                }
                if(peek("("))
                {
                    uint64_t v = functionPointerInt;
                    char * currNumToBranch = (char *)(v + (uint64_t) program);
                    uint64_t returnValForFunction = 0;
                    while(consume("("))
                    {
                        uint64_t currExpression = expression(effects);
                        consume(")");
                        stackPointer = stackPointer+1;
                        PCtoBranch[stackPointer] = (char *)current;
                        localIt[stackPointer] = currExpression;
                        current = currNumToBranch;
                        if(!consume("{"))
                        {
                            bool returned = false;
                            if(!statement(effects))
                            {
                                returned = true;
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        else
                        {
                            bool returned = false;
                            while(!consume("}"))
                            {
                                if(!statement(effects))
                                {
                                    returned = true;
                                    break;
                                }
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        returnValForFunction = returnRegister;
                        currNumToBranch = (char *)(returnValForFunction + (uint64_t) program);
                    }
                    return returnValForFunction;
                }
                else
                {
                    return functionPointerInt;
                }
            }
        }
        Slice* slicePtrTwo = consume_identifier();
        if (slicePtrTwo!=NULL) {
            if(effects)
            {
                uint64_t v = sliceToIntHashMapGet(s_table,slicePtrTwo);
                char * currNumToBranch = (char *)(v + (uint64_t) program);
                uint64_t returnValForFunction = 0;
                if(peek("("))
                {
                    
                    while(consume("("))
                    {
                        uint64_t currExpression = expression(effects);
                        consume(")");
                        stackPointer = stackPointer+1;
                        PCtoBranch[stackPointer] = (char *)current;
                        localIt[stackPointer] = currExpression;
                        current = currNumToBranch;
                        if(!consume("{"))
                        {
                            bool returned = false;
                            if(!statement(effects))
                            {
                                returned = true;
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        else
                        {
                            bool returned = false;
                            while(!consume("}"))
                            {
                                if(!statement(effects))
                                {
                                    returned = true;
                                    break;
                                }
                            }
                            if(!returned)
                            {   
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        returnValForFunction = returnRegister;
                        currNumToBranch = (char *)(returnValForFunction + (uint64_t) program);
                    }
                    return returnValForFunction;
                }
                else
                {
                    return v;
                }
                
            }
            else
            {
                if(peek("("))
                {
                    while(consume("("))
                    {
                        expression(effects);
                        consume(")");
                    }    
                }
                return 0;
            }   
        }
        uint64_t* cons_litPtr = consume_literal();
        if (cons_litPtr!=NULL) {
            uint64_t temp = *cons_litPtr;
            free(cons_litPtr);
            if(effects)
            {
                return temp;
            }
            else
            {
                return 0;
            }
        }
        if (consume("(")) {
            uint64_t v = expression(effects);
            consume(")");
            if(peek("("))
                {
                    char * currNumToBranch = (char *)(v + (uint64_t) program);
                    uint64_t returnValForFunction = 0;
                    while(consume("("))
                    {
                        uint64_t currExpression = expression(effects);
                        consume(")");
                        stackPointer = stackPointer+1;
                        PCtoBranch[stackPointer] = (char *)current;
                        localIt[stackPointer] = currExpression;
                        current = currNumToBranch;
                        if(!consume("{"))
                        {
                            bool returned = false;
                            if(!statement(effects))
                            {
                                returned = true;
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        else
                        {
                            bool returned = false;
                            while(!consume("}"))
                            {
                                if(!statement(effects))
                                {
                                    returned = true;
                                    break;
                                }
                            }
                            if(!returned)
                            {
                                if(peek("return"))
                                {
                                    statement(effects);
                                }
                                else
                                {
                                    current = PCtoBranch[stackPointer];
                                    PCtoBranch[stackPointer] = 0;
                                    localIt[stackPointer] = 0;
                                    stackPointer=stackPointer-1;
                                    returnRegister = 0;
                                }
                            }
                        }
                        returnValForFunction = returnRegister;
                        currNumToBranch = (char *)(returnValForFunction + (uint64_t) program);
                    }
                    return returnValForFunction;
                }
            return v;
        } 
        else {
            fail();
            return 0;
        }
        fail();
        return 0;
    }

    // ++ -- unary+ unary- ... (Right) deprecated
    uint64_t e2(bool effects) {
        return e1(effects);
    }
    
    // * / % (Left)
    uint64_t e3(bool effects) {
        uint64_t v = e2(effects);

        while (true) {
            if (consume("*")) {
                v = v * e2(effects);
            } else if (consume("/")) {
                uint64_t right = e2(effects);
                v = (right == 0) ? 0 : v / right;
            } else if (consume("%")) {
                uint64_t right = e2(effects);
                v = (right == 0) ? 0 : v % right;
            } else {
                return v;
            }
        }
    }

    // (Left) + -
    uint64_t e4(bool effects) {
        uint64_t v = e3(effects);
        //printf("%d\n",(int)v);
        while (true) {
            if (consume("+")) {
                v = v + e3(effects);
                //printf("%d\n",(int)v);
            } else if (consume("-")) {
                v = v - e3(effects);
                //printf("%d\n",(int)v);
            } else {
                return v;
            }
        }
    }

    // << >> deprecated
    uint64_t e5(bool effects) {
        return e4(effects);
    }

    //all the other arithematic/logical stuff is implemented the same was as the starter code.
    //a recursive call to the left, and then a recursive call to the right.

    // < <= > >=
    uint64_t e6(bool effects) {
        uint64_t v = e5(effects);
        while(true) {
            if(consume("<=")) {
                if(v<=e5(effects))
                {
                    v = 1;
                }
                else 
                {
                    v = 0;
                }
            }
            else if(consume("<"))  {
                if(v<e5(effects))
                {
                    v = 1;
                }
                else 
                {
                    v = 0;
                }
            }
            else if(consume(">=")) {
                if(v>=e5(effects))
                {
                    v = 1;
                }
                else 
                {
                    v = 0;
                }
            }
            else if(consume(">"))  {
                if(v>e5(effects))
                {
                    v = 1;
                }
                else 
                {
                    v = 0;
                }
            }
            else
            {
                return v;
            }
        }
    }

    // == !=
    uint64_t e7(bool effects) {
        uint64_t v = e6(effects);
        while(true)
        {
            if(consume("=="))
            {
                if(v==e6(effects)){
                    v=1;
                }
                else{
                    v=0;
                }
            }
            else if(consume("!=")){
                if(v!=e6(effects)){
                    v=1;
                }
                else{
                    v=0;
                }
            }
            else{
                return v;
            }
        }
    }

    // (left) &
    uint64_t e8(bool effects) {
        uint64_t v = e7(effects);
        while(true)
        {   
            char * checkerTwo = (char *) current;
            while(isspace(*checkerTwo))
            {
                checkerTwo +=1;
            }
            if(*checkerTwo=='&' && *(checkerTwo+1)=='&')
            {
                return v;
            }
            if(consume("&"))
            {
                v=v & e7(effects);
            }
            else{
                return v;
            }
        }
    }

    // ^
    uint64_t e9(bool effects) {
        uint64_t v = e8(effects);
        while(true)
        {
            if(consume("^"))
            {
                v=v ^ e8(effects);
            }
            else{
                return v;
            }
        }
    }

    // |
    uint64_t e10(bool effects) {
        uint64_t v = e9(effects);
        while(true)
        {
            char * checkerTwo = (char *) current;
            while(isspace(*checkerTwo))
            {
                checkerTwo +=1;
            }
            if(*checkerTwo=='|' && *(checkerTwo+1)=='|')
            {
                return v;
            }
            if(consume("|"))
            {
                v=v | e9(effects);
            }
            else{
                return v;
            }
        }
    }
    //short circuiting is done through an if statement, which would render the other half to have a false effect(only increment PC).
    // &&
    uint64_t e11(bool effects) {
        uint64_t v = e10(effects);
        while(true)
        {
            if(consume("&&"))
            {
                if(v<1)
                {
                    v=0;
                    e10(false);
                }
                else if(v&&e10(effects)){
                    v=1;
                }
                else{
                    v=0;
                }
            }
            else{
                return v;
            }
        }
    }

    // ||
    uint64_t e12(bool effects) {
        uint64_t v = e11(effects);
        while(true)
        {
            if(consume("||"))
            {
                if(v>=1)
                {
                    e11(false);
                    v=1;
                }
                else if(v||e11(effects)==1){
                    v=1;
                }
                else{
                    v=0;
                }
            }
            else{
                return v;
            }
        }
    }

    // (right with special treatment for middle expression) ?: deprecated
    uint64_t e13(bool effects) {
        return e12(effects);
    }

    // = += -= ... deprecated
    uint64_t e14(bool effects) {
        return e13(effects);
    }

    // ,
    uint64_t e15(bool effects) {
        uint64_t v = e14(effects);
        while(true)
        {
            if(consume(","))
            {
                v = e14(effects);
            }
            else{
                return v;
            }
        }
    }

    uint64_t expression(bool effects) {
        return e15(effects);
    }

    bool statement(bool effects) {

        //this deals with it as the start of a statement(aka a varaible), essentially implemented by just treating it as variable.
        if (consume("it")) {
            //printf("%s\n","traversed here it");
            if (consume("=")) {
                uint64_t v = expression(effects);
                if (effects) {

                    localIt[stackPointer] = v;
                }
                return true;
            } else {
                fail();
            }
        }
        //code here decrements the stack pointer, and then write the return value to the return register.
        if (consume("return")) {
            if(!effects)
            {
                expression(effects);
                return true;
            }
            uint64_t toRet = expression(effects);
            current = PCtoBranch[stackPointer];
            PCtoBranch[stackPointer] = 0;
            localIt[stackPointer] = 0;
            stackPointer=stackPointer - 1;
            returnRegister = toRet;
            return false;
        }
        if (consume("print")) {
            // print ...
            uint64_t v = expression(effects);
            if (effects) {
                printf("%ld\n",v);
            }
            return true;
        }
        //the while block here keeps a pointer to the start, and then constantly jumps to it until the logic fails
        if (consume("while"))
        {
            char * toGoBack = (char*) current;
            uint64_t toEval =  expression(effects);
            while(toEval>=1&&effects)
            {
                if(!consume("{"))
                {
                    if(peek("return"))
                    {
                        return true;
                    }
                    statement(effects);
                }
                else
                {
                    while(!consume("}"))
                    {
                        if(peek("return"))
                        {
                            return true;
                        }
                        statement(effects);
                    }
                }
                current = toGoBack;
                toEval =  expression(effects);
            }
            if(toEval<1||!effects)
            {
                //printf("%s\n","reached2");
                if(!consume("{"))
                {
                    statement(false);
                }
                else
                {
                    while(!consume("}"))
                    {
                        //printf("%s\n","line");
                        statement(false);
                    }
                }
            }
            return true;
        }
        //the if block would have effects treated as "true" if the logic inside is evaluated to be true, and vice verse.
        //else is implemented the same way
        if (consume("if")) 
        {
            //printf("%s\n","traversed here if");
            //printf("%s\n","reached");
            uint64_t toEval = expression(effects);
            //printf("%s\n","if val");
            //printf("%ld\n",toEval);
            if(toEval)
            {
                if(!consume("{"))
                {
                    //printf("%s\n","traversed here if block");
                    if(peek("return")&&effects)
                    {
                        //printf("%s\n","saw return");
                        //printf("%c\n", *current);
                        return true;
                    }
                    statement(effects);
                }
                else
                {
                    
                    while(!consume("}"))
                    {   
                        if(peek("return")&&effects)
                        {
                        //printf("%s\n","saw return");
                            return true;
                        } 
                        statement(effects);
                    }
                }
            }
            else
            {
                if(!consume("{"))
                {
                    statement(false);
                }
                else
                {
                    while(!consume("}"))
                    {
                        statement(false);
                    }
                }
            }
            if(consume("else"))
            {
                if(toEval<1)
                {
                    if(!consume("{"))
                    {
                        if(peek("return")&&effects)
                        {
                            //printf("%s\n","saw return");
                            return true;
                        }
                        statement(effects);
                    }
                    else
                    {
                        while(!consume("}"))
                        {
                            if(peek("return")&&effects)
                            {
                                //printf("%s\n","saw return");
                                return true;
                            }
                            statement(effects);
                        }
                    }
                }
                else
                {
                    if(!consume("{"))
                    {
                        statement(false);
                    }
                    else
                    {
                        while(!consume("}"))
                        {
                            statement(false);
                        }
                    }      
                }
            }
            return true;
        }
        Slice* slicePtrOne = consume_identifier();
        if (slicePtrOne!=NULL) {
            //printf("%s\n","traversed here slice");
            // x = ...
            if (consume("=")) {
                uint64_t v = expression(effects);
                /*
                if(effects)
                {
                    printf("%s\n", "true");
                }
                else
                {
                    printf("%s\n","false");
                }
                */
                if (effects) {
                    /*char* c = slicePtrOne->start;
                    int temp = 0;
                    while(c[temp]!=0)
                    {
                        printf("%c",c[temp]);
                        temp++;
                    }
                    printf("%ld",v);*/
                    sliceToIntHashMapInsert(s_table,slicePtrOne,v);
                }
                return true;
            } else {
                fail();
            }
        }
        //printf("%s\n","fail here");
        return false;
    }

    void statements(bool effects) {
        while (statement(effects))
        {
            //printf("%s\n","new statement");
        }
    }

    void run() {
        statements(true);
        end_or_fail();
    }


int main(int argc, const char *const *const argv) {

    if (argc != 2) {
        fprintf(stderr,"usage: %s <file name>\n",argv[0]);
        exit(1);
    }

    // open the file
    int fd = open(argv[1],O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // determine its size (std::filesystem::get_size?)
    struct stat file_stats;
    int rc = fstat(fd,&file_stats);
    if (rc != 0) {
        perror("fstat");
        exit(1);
    }

    // map the file in my address space
    char const* prog = (char const *)mmap(
        0,
        file_stats.st_size,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0);
    if (prog == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    program = prog;
    current =  prog;

    s_table = createSliceToIntHashMap(1000);

    
    run();
    //current = prog;
    //run();
    /*
    for(int i=0;i<1000;i++)
    {
        if(s_table->arrStart[i]!=0)
        {
            sliceNode* currNode = s_table->arrStart[i];
            while(currNode!=0)
            {
                char* c = currNode->key->start;
                int len = (int) currNode->key->len;
                for(int i=0;i<len;i++)
                {
                    printf("%c",c[i]);
                }
                currNode = currNode->nextNode;
            }
            printf("%ld",(long)s_table->arrStart[i]->val);
        }
    }
    */
    return 0;
}

// vim: tabstop=4 shiftwidth=2 expandtab
