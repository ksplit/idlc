

	#include "../ast/ast.h"


#include <stdint.h>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>


namespace Parser{



struct Value{
    typedef std::list<Value>::const_iterator iterator;

    Value():
        which(1),
        value(0){
    }

    Value(const Value & him):
    which(him.which),
    value(0){
        if (him.isData()){
            value = him.value;
        }
        if (him.isList()){
            values = him.values;
        }
    }

    explicit Value(const void * value):
        which(0),
        value(value){
    }

    Value & operator=(const Value & him){
        which = him.which;
        if (him.isData()){
            value = him.value;
        }
        if (him.isList()){
            values = him.values;
        }
        return *this;
    }

    Value & operator=(const void * what){
        this->value = what;
        return *this;
    }

    void reset(){
        this->value = 0;
        this->values.clear();
        this->which = 1;
    }

    int which; // 0 is value, 1 is values

    inline bool isList() const {
        return which == 1;
    }

    inline bool isData() const {
        return which == 0;
    }

    inline const void * getValue() const {
        return value;
    }

    inline void setValue(const void * value){
        which = 0;
        this->value = value;
    }

    inline const std::list<Value> & getValues() const {
        return values;
    }

    /*
    inline void setValues(std::list<Value> values){
        which = 1;
        values = values;
    }
    */

    const void * value;
    std::list<Value> values;
};



class Result{
public:
    Result():
    position(-2){
    }

    Result(const int position):
    position(position){
    }

    Result(const Result & r):
    position(r.position),
    value(r.value){
    }

    Result & operator=(const Result & r){
        position = r.position;
        value = r.value;
        
        return *this;
    }

    void reset(){
        value.reset();
    }

    void setPosition(int position){
        this->position = position;
    }

    inline int getPosition() const {
        return position;
    }

    inline bool error(){
        return position == -1;
    }

    inline bool calculated(){
        return position != -2;
    }

    
    inline void nextPosition(){
        position += 1;
    }

    void setError(){
        position = -1;
    }

    inline void setValue(const Value & value){
        this->value = value;
    }

    /*
    Value getLastValue() const {
        if (value.isList()){
            if (value.values.size() == 0){
                std::cout << "[peg] No last value to get!" << std::endl;
            }
            return value.values[value.values.size()-1];
        } else {
            return value;
        }
    }
    */

    inline int matches() const {
        if (value.isList()){
            return this->value.values.size();
        } else {
            return 1;
        }
    }

    inline const Value & getValues() const {
        return this->value;
    }

    void addResult(const Result & result){
        std::list<Value> & mine = this->value.values;
        mine.push_back(result.getValues());
        this->position = result.getPosition();
        this->value.which = 1;
    }

private:
    int position;
    Value value;
    
};


struct Chunk0{
Result chunk_tok_ident;
    Result chunk_tok_space;
    Result chunk_tok_path;
    Result chunk_tok_string;
    Result chunk_lex_string_tail;
};

struct Chunk1{
Result chunk_lex_path_tail;
    Result chunk_lex_line_comment_tail;
    Result chunk_lex_line_comment;
    Result chunk_lex_block_comment;
    Result chunk_lex_block_comment_tail;
};

struct Chunk2{
Result chunk_lex_comment;
    Result chunk_lex_ident_rest;
    Result chunk_lex_keyword;
    Result chunk_file;
    Result chunk_driver_file;
};

struct Chunk3{
Result chunk_include_list;
    Result chunk_include_stmt;
    Result chunk_driver_def;
    Result chunk_import_list;
    Result chunk_import_stmt;
};

struct Chunk4{
Result chunk_module_file;
    Result chunk_module_list;
    Result chunk_module_def;
    Result chunk_module_item_list;
    Result chunk_module_item;
};

struct Chunk5{
Result chunk_header_stmt;
    Result chunk_proj_def;
    Result chunk_proj_type;
    Result chunk_proj_struct_type;
    Result chunk_proj_union_type;
};

struct Chunk6{
Result chunk_field_ref;
    Result chunk_field_rel_ref;
    Result chunk_field_abs_ref;
    Result chunk_proj_field_list;
    Result chunk_proj_field;
};

struct Chunk7{
Result chunk_typename_ptr;
    Result chunk_val_attr;
    Result chunk_rpc_side_spec;
    Result chunk_ptr_attr;
    Result chunk_val_attrs;
};

struct Chunk8{
Result chunk_ptr_attrs;
    Result chunk_val_attr_list;
    Result chunk_ptr_attr_list;
    Result chunk_var_decl;
    Result chunk_typename_any_no_ptr;
};

struct Chunk9{
Result chunk_typename_any;
    Result chunk_typename_arith;
    Result chunk_typename_array;
    Result chunk_array_size;
    Result chunk_typename_string;
};

struct Chunk10{
Result chunk_typename_rpc;
    Result chunk_typename_proj;
    Result chunk_rpc_def;
    Result chunk_rpc_item_list;
    Result chunk_rpc_item;
};

struct Column{
    Column():
    chunk0(0)
        ,chunk1(0)
        ,chunk2(0)
        ,chunk3(0)
        ,chunk4(0)
        ,chunk5(0)
        ,chunk6(0)
        ,chunk7(0)
        ,chunk8(0)
        ,chunk9(0)
        ,chunk10(0){
    }

    Chunk0 * chunk0;
    Chunk1 * chunk1;
    Chunk2 * chunk2;
    Chunk3 * chunk3;
    Chunk4 * chunk4;
    Chunk5 * chunk5;
    Chunk6 * chunk6;
    Chunk7 * chunk7;
    Chunk8 * chunk8;
    Chunk9 * chunk9;
    Chunk10 * chunk10;

    int hitCount(){
        return 0;
    }

    int maxHits(){
        return 55;
    }

    ~Column(){
        delete chunk0;
        delete chunk1;
        delete chunk2;
        delete chunk3;
        delete chunk4;
        delete chunk5;
        delete chunk6;
        delete chunk7;
        delete chunk8;
        delete chunk9;
        delete chunk10;
    }
};


class ParseException: std::exception {
public:
    ParseException(const std::string & reason):
    std::exception(),
    line(-1), column(-1),
    message(reason){
    }

    ParseException(const std::string & reason, int line, int column):
    std::exception(),
    line(line), column(column),
    message(reason){
    }

    std::string getReason() const;
    int getLine() const;
    int getColumn() const;

    virtual ~ParseException() throw(){
    }

protected:
    int line, column;
    std::string message;
};

class Stream{
public:
    struct LineInfo{
        LineInfo(int line, int column):
        line(line),
        column(column){
        }

        LineInfo(const LineInfo & copy):
        line(copy.line),
        column(copy.column){
        }

        LineInfo():
        line(-1),
        column(-1){
        }

        int line;
        int column;
    };

public:
    /* read from a file */
    Stream(const std::string & filename):
    temp(0),
    buffer(0),
    farthest(0),
    last_line_info(-1){
        std::ifstream stream;
        /* ios::binary is needed on windows */
        stream.open(filename.c_str(), std::ios::in | std::ios::binary);
        if (stream.fail()){
            std::ostringstream out;
            out << __FILE__  << " cannot open '" << filename << "'";
            throw ParseException(out.str());
        }
        stream.seekg(0, std::ios_base::end);
        max = stream.tellg();
        stream.seekg(0, std::ios_base::beg);
        temp = new char[max];
        stream.read(temp, max);
        buffer = temp;
        stream.close();

        line_info[-1] = LineInfo(1, 1);

        setup();
    }

    /* for null-terminated strings */
    Stream(const char * in):
    temp(0),
    buffer(in),
    farthest(0),
    last_line_info(-1){
        max = strlen(buffer);
        line_info[-1] = LineInfo(1, 1);
        setup();
    }

    /* user-defined length */
    Stream(const char * in, int length):
    temp(0),
    buffer(in),
    farthest(0),
    last_line_info(-1){
        max = length;
        line_info[-1] = LineInfo(1, 1);
        setup();
    }

    void setup(){
        
        createMemo();
    }

    void createMemo(){
        memo_size = 1024 * 2;
        memo = new Column*[memo_size];
        /* dont create column objects before they are needed because transient
         * productions will never call for them so we can save some space by
         * not allocating columns at all.
         */
        memset(memo, 0, sizeof(Column*) * memo_size);
    }

    int length(){
        return max;
    }

    /* prints statistics about how often rules were fired and how
     * likely rules are to succeed
     */
    void printStats(){
        double min = 1;
        double max = 0;
        double average = 0;
        int count = 0;
        for (int i = 0; i < length(); i++){
            Column & c = getColumn(i);
            double rate = (double) c.hitCount() / (double) c.maxHits();
            if (rate != 0 && rate < min){
                min = rate;
            }
            if (rate > max){
                max = rate;
            }
            if (rate != 0){
                average += rate;
                count += 1;
            }
        }
        std::cout << "Min " << (100 * min) << " Max " << (100 * max) << " Average " << (100 * average / count) << " Count " << count << " Length " << length() << " Rule rate " << (100.0 * (double)count / (double) length()) << std::endl;
    }

    char get(const int position){
        if (position >= max || position < 0){
            return '\0';
        }

        // std::cout << "Read char '" << buffer[position] << "'" << std::endl;

        return buffer[position];
    }

    bool find(const char * str, const int position){
        if (position >= max || position < 0){
            return false;
        }
        return strncmp(&buffer[position], str, max - position) == 0;
    }

    void growMemo(){
        int newSize = memo_size * 2;
        Column ** newMemo = new Column*[newSize];
        /* Copy old memo table */
        memcpy(newMemo, memo, sizeof(Column*) * memo_size);

        /* Zero out new entries */
        memset(&newMemo[memo_size], 0, sizeof(Column*) * (newSize - memo_size));

        /* Delete old memo table */
        delete[] memo;

        /* Set up new memo table */
        memo = newMemo;
        memo_size = newSize;
    }

    /* I'm sure this can be optimized. It only takes into account
     * the last position used to get line information rather than
     * finding a position closest to the one asked for.
     * So if the last position is 20 and the current position being requested
     * is 15 then this function will compute the information starting from 0.
     * If the information for 10 was computed then that should be used instead.
     * Maybe something like, sort the positions, find closest match lower
     * than the position and start from there.
     */
    LineInfo makeLineInfo(int last_line_position, int position){
        int line = line_info[last_line_position].line;
        int column = line_info[last_line_position].column;
        for (int i = last_line_position + 1; i < position; i++){
            if (buffer[i] == '\n'){
                line += 1;
                column = 1;
            } else {
                column += 1;
            }
        }
        return LineInfo(line, column);
    }

    void updateLineInfo(int position){
        if (line_info.find(position) == line_info.end()){
            if (position > last_line_info){
                line_info[position] = makeLineInfo(last_line_info, position);
            } else {
                line_info[position] = makeLineInfo(0, position);
            }
            last_line_info = position;
        }
    }

    const LineInfo & getLineInfo(int position){
        updateLineInfo(position);
        return line_info[position];
    }

    /* throws a ParseException */
    void reportError(const std::string & parsingContext){
        std::ostringstream out;
        int line = 1;
        int column = 1;
        for (int i = 0; i < farthest; i++){
            if (buffer[i] == '\n'){
                line += 1;
                column = 1;
            } else {
                column += 1;
            }
        }
        int context = 15;
        int left = farthest - context;
        int right = farthest + context;
        if (left < 0){
            left = 0;
        }
        if (right >= max){
            right = max;
        }
        out << "Error while parsing " << parsingContext << ". Read up till line " << line << " column " << column << std::endl;
        std::ostringstream show;
        for (int i = left; i < right; i++){
            char c = buffer[i];
            switch (buffer[i]){
                case '\n' : {
                    show << '\\';
                    show << 'n';
                    break;
                }
                case '\r' : {
                    show << '\\';
                    show << 'r';
                    break;
                }
                case '\t' : {
                    show << '\\';
                    show << 't';
                    break;
                }
                default : show << c; break;
            }
        }
        out << "'" << show.str() << "'" << std::endl;
        for (int i = 0; i < farthest - left; i++){
            out << " ";
        }
        out << "^" << std::endl;
        out << "Last successful rule trace" << std::endl;
        out << makeBacktrace(last_trace) << std::endl;
        throw ParseException(out.str(), line, column);
    }

    std::string makeBacktrace(){
        return makeBacktrace(rule_backtrace);
    }

    std::string makeBacktrace(const std::vector<std::string> & trace){
        std::ostringstream out;

        bool first = true;
        for (std::vector<std::string>::const_iterator it = trace.begin(); it != trace.end(); it++){
            if (!first){
                out << " -> ";
            } else {
                first = false;
            }
            out << *it;
        }

        return out.str();
    }

    inline Column & getColumn(const int position){
        while (position >= memo_size){
            growMemo();
        }
        /* create columns lazily because not every position will have a column. */
        if (memo[position] == NULL){
            memo[position] = new Column();
        }
        return *(memo[position]);
    }

    void update(const int position){
        if (position > farthest){
            farthest = position;
            last_trace = rule_backtrace;
        }
    }

    void push_rule(const std::string & name){
        rule_backtrace.push_back(name);
    }

    void pop_rule(){
        rule_backtrace.pop_back();
    }

    

    ~Stream(){
        delete[] temp;
        for (int i = 0; i < memo_size; i++){
            delete memo[i];
        }
        delete[] memo;
    }

private:
    char * temp;
    const char * buffer;
    /* an array is faster and uses less memory than std::map */
    Column ** memo;
    int memo_size;
    int max;
    int farthest;
    std::vector<std::string> rule_backtrace;
    std::vector<std::string> last_trace;
    int last_line_info;
    std::map<int, LineInfo> line_info;
    
};

static int getCurrentLine(const Value & value){
    Stream::LineInfo * info = (Stream::LineInfo*) value.getValue();
    return info->line;
}

static int getCurrentColumn(const Value & value){
    Stream::LineInfo * info = (Stream::LineInfo*) value.getValue();
    return info->column;
}

class RuleTrace{
public:
    RuleTrace(Stream & stream, const char * name):
    stream(stream){
        stream.push_rule(name);
    }

    void setName(const std::string & name){
        stream.pop_rule();
        stream.push_rule(name);
    }

    ~RuleTrace(){
        stream.pop_rule();
    }

    Stream & stream;
};

static inline bool compareChar(const char a, const char b){
    return a == b;
}

static inline char lower(const char x){
    if (x >= 'A' && x <= 'Z'){
        return x - 'A' + 'a';
    }
    return x;
}

static inline bool compareCharCase(const char a, const char b){
    return lower(a) == lower(b);
}


std::string ParseException::getReason() const {
    return message;
}

int ParseException::getLine() const {
    return line;
}

int ParseException::getColumn() const {
    return column;
}

Result errorResult(-1);

Result rule_tok_ident(Stream &, const int);
Result rule_tok_space(Stream &, const int);
Result rule_tok_path(Stream &, const int);
Result rule_tok_string(Stream &, const int);
Result rule_lex_string_tail(Stream &, const int);
Result rule_lex_path_tail(Stream &, const int);
Result rule_lex_line_comment_tail(Stream &, const int);
Result rule_lex_line_comment(Stream &, const int);
Result rule_lex_block_comment(Stream &, const int);
Result rule_lex_block_comment_tail(Stream &, const int);
Result rule_lex_comment(Stream &, const int);
Result rule_lex_ident_rest(Stream &, const int);
Result rule_lex_keyword(Stream &, const int);
Result rule_file(Stream &, const int);
Result rule_driver_file(Stream &, const int);
Result rule_include_list(Stream &, const int);
Result rule_include_stmt(Stream &, const int);
Result rule_driver_def(Stream &, const int);
Result rule_import_list(Stream &, const int);
Result rule_import_stmt(Stream &, const int);
Result rule_module_file(Stream &, const int);
Result rule_module_list(Stream &, const int);
Result rule_module_def(Stream &, const int);
Result rule_module_item_list(Stream &, const int);
Result rule_module_item(Stream &, const int);
Result rule_header_stmt(Stream &, const int);
Result rule_proj_def(Stream &, const int);
Result rule_proj_type(Stream &, const int);
Result rule_proj_struct_type(Stream &, const int);
Result rule_proj_union_type(Stream &, const int);
Result rule_field_ref(Stream &, const int);
Result rule_field_rel_ref(Stream &, const int);
Result rule_field_abs_ref(Stream &, const int);
Result rule_proj_field_list(Stream &, const int);
Result rule_proj_field(Stream &, const int);
Result rule_typename_ptr(Stream &, const int);
Result rule_val_attr(Stream &, const int);
Result rule_rpc_side_spec(Stream &, const int);
Result rule_ptr_attr(Stream &, const int);
Result rule_val_attrs(Stream &, const int);
Result rule_ptr_attrs(Stream &, const int);
Result rule_val_attr_list(Stream &, const int);
Result rule_ptr_attr_list(Stream &, const int);
Result rule_var_decl(Stream &, const int);
Result rule_typename_any_no_ptr(Stream &, const int);
Result rule_typename_any(Stream &, const int);
Result rule_typename_arith(Stream &, const int);
Result rule_typename_array(Stream &, const int);
Result rule_array_size(Stream &, const int);
Result rule_typename_string(Stream &, const int);
Result rule_typename_rpc(Stream &, const int);
Result rule_typename_proj(Stream &, const int);
Result rule_rpc_def(Stream &, const int);
Result rule_rpc_item_list(Stream &, const int);
Result rule_rpc_item(Stream &, const int);




Result rule_tok_ident(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_tok_ident.calculated()){
        return column_peg_2.chunk0->chunk_tok_ident;
    }
    
    
    RuleTrace trace_peg_1(stream, "tok_ident");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_lex_keyword(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            {
                    bool ok = true;
                    ok = false;
                    if (!ok){
                        goto out_peg_5;
                    }
                }
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_ident = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
        Result result_peg_6(myposition);
        
        
        {
        
            {
                    int position_peg_9 = result_peg_6.getPosition();
                    
                    char letter_peg_10 = stream.get(result_peg_6.getPosition());
                    if (letter_peg_10 != '\0' && strchr("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", letter_peg_10) != NULL){
                        result_peg_6.nextPosition();
                        result_peg_6.setValue(Value((void*) (intptr_t) letter_peg_10));
                    } else {
                        result_peg_6.setPosition(position_peg_9);
                        goto out_peg_11;
                    }
                    
                }
                goto success_peg_8;
                out_peg_11:
                goto out_peg_12;
                success_peg_8:
                ;
            
            
            
            result_peg_6.reset();
                do{
                    Result result_peg_14(result_peg_6.getPosition());
                    result_peg_14 = rule_lex_ident_rest(stream, result_peg_14.getPosition());
                    if (result_peg_14.error()){
                        goto loop_peg_13;
                    }
                    result_peg_6.addResult(result_peg_14);
                } while (true);
                loop_peg_13:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_ident = result_peg_6;
        stream.update(result_peg_6.getPosition());
        
        
        return result_peg_6;
        out_peg_12:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_ident = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_tok_space(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_tok_space.calculated()){
        return column_peg_2.chunk0->chunk_tok_space;
    }
    
    
    RuleTrace trace_peg_1(stream, "tok_space");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.reset();
        do{
            Result result_peg_5(result_peg_3.getPosition());
            {
                int position_peg_7 = result_peg_5.getPosition();
                
                char letter_peg_8 = stream.get(result_peg_5.getPosition());
                if (letter_peg_8 != '\0' && strchr(" \t\n\r", letter_peg_8) != NULL){
                    result_peg_5.nextPosition();
                    result_peg_5.setValue(Value((void*) (intptr_t) letter_peg_8));
                } else {
                    result_peg_5.setPosition(position_peg_7);
                    goto out_peg_9;
                }
                
            }
            goto success_peg_6;
            out_peg_9:
            goto loop_peg_4;
            success_peg_6:
            ;
            result_peg_3.addResult(result_peg_5);
        } while (true);
        loop_peg_4:
        if (result_peg_3.matches() == 0){
            goto out_peg_10;
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_space = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_10:
        Result result_peg_11(myposition);
        
        
        result_peg_11 = rule_lex_comment(stream, result_peg_11.getPosition());
        if (result_peg_11.error()){
            goto out_peg_12;
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_space = result_peg_11;
        stream.update(result_peg_11.getPosition());
        
        
        return result_peg_11;
        out_peg_12:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_space = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_tok_path(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_tok_path.calculated()){
        return column_peg_2.chunk0->chunk_tok_path;
    }
    
    
    RuleTrace trace_peg_1(stream, "tok_path");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3.setValue(Value((void*) "<"));
                for (int i = 0; i < 1; i++){
                    if (compareChar("<"[i], stream.get(result_peg_3.getPosition()))){
                        result_peg_3.nextPosition();
                    } else {
                        goto out_peg_5;
                    }
                }
            
            
            
            result_peg_3 = rule_lex_path_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_path = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_path = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_tok_string(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_tok_string.calculated()){
        return column_peg_2.chunk0->chunk_tok_string;
    }
    
    
    RuleTrace trace_peg_1(stream, "tok_string");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3.setValue(Value((void*) 34));
                if ((unsigned char) stream.get(result_peg_3.getPosition()) == (unsigned char) 34){
                    result_peg_3.nextPosition();
                } else {
                    goto out_peg_5;
                }
            
            
            
            result_peg_3 = rule_lex_string_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_string = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_tok_string = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_string_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_lex_string_tail.calculated()){
        return column_peg_2.chunk0->chunk_lex_string_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_string_tail");
    int myposition = position;
    
    tail_peg_10:
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.setValue(Value((void*) 34));
        if ((unsigned char) stream.get(result_peg_3.getPosition()) == (unsigned char) 34){
            result_peg_3.nextPosition();
        } else {
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_lex_string_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        {
        
            char letter_peg_7 = stream.get(result_peg_5.getPosition());
                if (letter_peg_7 != '\0' && strchr("\n\r", letter_peg_7) != NULL){
                    result_peg_5.nextPosition();
                    result_peg_5.setValue(Value((void*) (intptr_t) letter_peg_7));
                } else {
                    goto out_peg_8;
                }
            
            
            
            {
                    bool ok = true;
                    ok = false;
                    if (!ok){
                        goto out_peg_8;
                    }
                }
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_lex_string_tail = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_8:
        Result result_peg_9(myposition);
        {
        
            char temp_peg_12 = stream.get(result_peg_9.getPosition());
                if (temp_peg_12 != '\0'){
                    result_peg_9.setValue(Value((void*) (intptr_t) temp_peg_12));
                    result_peg_9.nextPosition();
                } else {
                    goto out_peg_13;
                }
            
            
            
            
            
            
        }
        myposition = result_peg_9.getPosition();
        goto tail_peg_10;
        out_peg_13:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_lex_string_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_path_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_lex_path_tail.calculated()){
        return column_peg_2.chunk1->chunk_lex_path_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_path_tail");
    int myposition = position;
    
    tail_peg_6:
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.setValue(Value((void*) ">"));
        for (int i = 0; i < 1; i++){
            if (compareChar(">"[i], stream.get(result_peg_3.getPosition()))){
                result_peg_3.nextPosition();
            } else {
                goto out_peg_4;
            }
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_path_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        {
        
            char temp_peg_8 = stream.get(result_peg_5.getPosition());
                if (temp_peg_8 != '\0'){
                    result_peg_5.setValue(Value((void*) (intptr_t) temp_peg_8));
                    result_peg_5.nextPosition();
                } else {
                    goto out_peg_9;
                }
            
            
            
            
            
            
        }
        myposition = result_peg_5.getPosition();
        goto tail_peg_6;
        out_peg_9:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_path_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_line_comment_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_lex_line_comment_tail.calculated()){
        return column_peg_2.chunk1->chunk_lex_line_comment_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_line_comment_tail");
    int myposition = position;
    
    tail_peg_6:
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.setValue(Value((void*) "\n"));
        for (int i = 0; i < 1; i++){
            if (compareChar("\n"[i], stream.get(result_peg_3.getPosition()))){
                result_peg_3.nextPosition();
            } else {
                goto out_peg_4;
            }
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_line_comment_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        {
        
            char temp_peg_8 = stream.get(result_peg_5.getPosition());
                if (temp_peg_8 != '\0'){
                    result_peg_5.setValue(Value((void*) (intptr_t) temp_peg_8));
                    result_peg_5.nextPosition();
                } else {
                    goto out_peg_9;
                }
            
            
            
            
            
            
        }
        myposition = result_peg_5.getPosition();
        goto tail_peg_6;
        out_peg_9:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_line_comment_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_line_comment(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_lex_line_comment.calculated()){
        return column_peg_2.chunk1->chunk_lex_line_comment;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_line_comment");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3.setValue(Value((void*) "//"));
                for (int i = 0; i < 2; i++){
                    if (compareChar("//"[i], stream.get(result_peg_3.getPosition()))){
                        result_peg_3.nextPosition();
                    } else {
                        goto out_peg_5;
                    }
                }
            
            
            
            result_peg_3 = rule_lex_line_comment_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_line_comment = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_line_comment = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_block_comment(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_lex_block_comment.calculated()){
        return column_peg_2.chunk1->chunk_lex_block_comment;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_block_comment");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3.setValue(Value((void*) "/*"));
                for (int i = 0; i < 2; i++){
                    if (compareChar("/*"[i], stream.get(result_peg_3.getPosition()))){
                        result_peg_3.nextPosition();
                    } else {
                        goto out_peg_5;
                    }
                }
            
            
            
            result_peg_3 = rule_lex_block_comment_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_block_comment = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_block_comment = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_block_comment_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_lex_block_comment_tail.calculated()){
        return column_peg_2.chunk1->chunk_lex_block_comment_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_block_comment_tail");
    int myposition = position;
    
    tail_peg_6:
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.setValue(Value((void*) "*/"));
        for (int i = 0; i < 2; i++){
            if (compareChar("*/"[i], stream.get(result_peg_3.getPosition()))){
                result_peg_3.nextPosition();
            } else {
                goto out_peg_4;
            }
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_block_comment_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        {
        
            char temp_peg_8 = stream.get(result_peg_5.getPosition());
                if (temp_peg_8 != '\0'){
                    result_peg_5.setValue(Value((void*) (intptr_t) temp_peg_8));
                    result_peg_5.nextPosition();
                } else {
                    goto out_peg_9;
                }
            
            
            
            
            
            
        }
        myposition = result_peg_5.getPosition();
        goto tail_peg_6;
        out_peg_9:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_lex_block_comment_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_comment(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_lex_comment.calculated()){
        return column_peg_2.chunk2->chunk_lex_comment;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_comment");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_lex_line_comment(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_comment = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_lex_block_comment(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_comment = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_comment = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_ident_rest(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_lex_ident_rest.calculated()){
        return column_peg_2.chunk2->chunk_lex_ident_rest;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_ident_rest");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        char letter_peg_4 = stream.get(result_peg_3.getPosition());
        if (letter_peg_4 != '\0' && strchr("01234567890", letter_peg_4) != NULL){
            result_peg_3.nextPosition();
            result_peg_3.setValue(Value((void*) (intptr_t) letter_peg_4));
        } else {
            goto out_peg_5;
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_ident_rest = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
        Result result_peg_6(myposition);
        
        
        {
            int position_peg_8 = result_peg_6.getPosition();
            
            char letter_peg_9 = stream.get(result_peg_6.getPosition());
            if (letter_peg_9 != '\0' && strchr("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", letter_peg_9) != NULL){
                result_peg_6.nextPosition();
                result_peg_6.setValue(Value((void*) (intptr_t) letter_peg_9));
            } else {
                result_peg_6.setPosition(position_peg_8);
                goto out_peg_10;
            }
            
        }
        goto success_peg_7;
        out_peg_10:
        goto out_peg_11;
        success_peg_7:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_ident_rest = result_peg_6;
        stream.update(result_peg_6.getPosition());
        
        
        return result_peg_6;
        out_peg_11:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_ident_rest = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_lex_keyword(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_lex_keyword.calculated()){
        return column_peg_2.chunk2->chunk_lex_keyword;
    }
    
    
    RuleTrace trace_peg_1(stream, "lex_keyword");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "const"));
            for (int i = 0; i < 5; i++){
                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                    result_peg_3.nextPosition();
                } else {
                    result_peg_3.setPosition(position_peg_5);
                    goto out_peg_6;
                }
            }
                
        }
        goto success_peg_4;
        out_peg_6:
        goto out_peg_7;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "string"));
            for (int i = 0; i < 6; i++){
                if (compareChar("string"[i], stream.get(result_peg_8.getPosition()))){
                    result_peg_8.nextPosition();
                } else {
                    result_peg_8.setPosition(position_peg_10);
                    goto out_peg_11;
                }
            }
                
        }
        goto success_peg_9;
        out_peg_11:
        goto out_peg_12;
        success_peg_9:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
        Result result_peg_13(myposition);
        
        
        {
            int position_peg_15 = result_peg_13.getPosition();
            
            result_peg_13.setValue(Value((void*) "projection"));
            for (int i = 0; i < 10; i++){
                if (compareChar("projection"[i], stream.get(result_peg_13.getPosition()))){
                    result_peg_13.nextPosition();
                } else {
                    result_peg_13.setPosition(position_peg_15);
                    goto out_peg_16;
                }
            }
                
        }
        goto success_peg_14;
        out_peg_16:
        goto out_peg_17;
        success_peg_14:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_13;
        stream.update(result_peg_13.getPosition());
        
        
        return result_peg_13;
        out_peg_17:
        Result result_peg_18(myposition);
        
        
        {
            int position_peg_20 = result_peg_18.getPosition();
            
            result_peg_18.setValue(Value((void*) "rpc"));
            for (int i = 0; i < 3; i++){
                if (compareChar("rpc"[i], stream.get(result_peg_18.getPosition()))){
                    result_peg_18.nextPosition();
                } else {
                    result_peg_18.setPosition(position_peg_20);
                    goto out_peg_21;
                }
            }
                
        }
        goto success_peg_19;
        out_peg_21:
        goto out_peg_22;
        success_peg_19:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_18;
        stream.update(result_peg_18.getPosition());
        
        
        return result_peg_18;
        out_peg_22:
        Result result_peg_23(myposition);
        
        
        {
            int position_peg_25 = result_peg_23.getPosition();
            
            result_peg_23.setValue(Value((void*) "driver"));
            for (int i = 0; i < 6; i++){
                if (compareChar("driver"[i], stream.get(result_peg_23.getPosition()))){
                    result_peg_23.nextPosition();
                } else {
                    result_peg_23.setPosition(position_peg_25);
                    goto out_peg_26;
                }
            }
                
        }
        goto success_peg_24;
        out_peg_26:
        goto out_peg_27;
        success_peg_24:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_23;
        stream.update(result_peg_23.getPosition());
        
        
        return result_peg_23;
        out_peg_27:
        Result result_peg_28(myposition);
        
        
        {
            int position_peg_30 = result_peg_28.getPosition();
            
            result_peg_28.setValue(Value((void*) "module"));
            for (int i = 0; i < 6; i++){
                if (compareChar("module"[i], stream.get(result_peg_28.getPosition()))){
                    result_peg_28.nextPosition();
                } else {
                    result_peg_28.setPosition(position_peg_30);
                    goto out_peg_31;
                }
            }
                
        }
        goto success_peg_29;
        out_peg_31:
        goto out_peg_32;
        success_peg_29:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_28;
        stream.update(result_peg_28.getPosition());
        
        
        return result_peg_28;
        out_peg_32:
        Result result_peg_33(myposition);
        
        
        {
            int position_peg_35 = result_peg_33.getPosition();
            
            result_peg_33.setValue(Value((void*) "header"));
            for (int i = 0; i < 6; i++){
                if (compareChar("header"[i], stream.get(result_peg_33.getPosition()))){
                    result_peg_33.nextPosition();
                } else {
                    result_peg_33.setPosition(position_peg_35);
                    goto out_peg_36;
                }
            }
                
        }
        goto success_peg_34;
        out_peg_36:
        goto out_peg_37;
        success_peg_34:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_33;
        stream.update(result_peg_33.getPosition());
        
        
        return result_peg_33;
        out_peg_37:
        Result result_peg_38(myposition);
        
        
        {
            int position_peg_40 = result_peg_38.getPosition();
            
            result_peg_38.setValue(Value((void*) "import"));
            for (int i = 0; i < 6; i++){
                if (compareChar("import"[i], stream.get(result_peg_38.getPosition()))){
                    result_peg_38.nextPosition();
                } else {
                    result_peg_38.setPosition(position_peg_40);
                    goto out_peg_41;
                }
            }
                
        }
        goto success_peg_39;
        out_peg_41:
        goto out_peg_42;
        success_peg_39:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_38;
        stream.update(result_peg_38.getPosition());
        
        
        return result_peg_38;
        out_peg_42:
        Result result_peg_43(myposition);
        
        
        {
            int position_peg_45 = result_peg_43.getPosition();
            
            result_peg_43.setValue(Value((void*) "signature"));
            for (int i = 0; i < 9; i++){
                if (compareChar("signature"[i], stream.get(result_peg_43.getPosition()))){
                    result_peg_43.nextPosition();
                } else {
                    result_peg_43.setPosition(position_peg_45);
                    goto out_peg_46;
                }
            }
                
        }
        goto success_peg_44;
        out_peg_46:
        goto out_peg_47;
        success_peg_44:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_43;
        stream.update(result_peg_43.getPosition());
        
        
        return result_peg_43;
        out_peg_47:
        Result result_peg_48(myposition);
        
        
        {
            int position_peg_50 = result_peg_48.getPosition();
            
            result_peg_48.setValue(Value((void*) "struct"));
            for (int i = 0; i < 6; i++){
                if (compareChar("struct"[i], stream.get(result_peg_48.getPosition()))){
                    result_peg_48.nextPosition();
                } else {
                    result_peg_48.setPosition(position_peg_50);
                    goto out_peg_51;
                }
            }
                
        }
        goto success_peg_49;
        out_peg_51:
        goto out_peg_52;
        success_peg_49:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_48;
        stream.update(result_peg_48.getPosition());
        
        
        return result_peg_48;
        out_peg_52:
        Result result_peg_53(myposition);
        
        
        {
            int position_peg_55 = result_peg_53.getPosition();
            
            result_peg_53.setValue(Value((void*) "union"));
            for (int i = 0; i < 5; i++){
                if (compareChar("union"[i], stream.get(result_peg_53.getPosition()))){
                    result_peg_53.nextPosition();
                } else {
                    result_peg_53.setPosition(position_peg_55);
                    goto out_peg_56;
                }
            }
                
        }
        goto success_peg_54;
        out_peg_56:
        goto out_peg_57;
        success_peg_54:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_53;
        stream.update(result_peg_53.getPosition());
        
        
        return result_peg_53;
        out_peg_57:
        Result result_peg_58(myposition);
        
        
        {
            int position_peg_60 = result_peg_58.getPosition();
            
            result_peg_58.setValue(Value((void*) "char"));
            for (int i = 0; i < 4; i++){
                if (compareChar("char"[i], stream.get(result_peg_58.getPosition()))){
                    result_peg_58.nextPosition();
                } else {
                    result_peg_58.setPosition(position_peg_60);
                    goto out_peg_61;
                }
            }
                
        }
        goto success_peg_59;
        out_peg_61:
        goto out_peg_62;
        success_peg_59:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_58;
        stream.update(result_peg_58.getPosition());
        
        
        return result_peg_58;
        out_peg_62:
        Result result_peg_63(myposition);
        
        
        {
            int position_peg_65 = result_peg_63.getPosition();
            
            result_peg_63.setValue(Value((void*) "short"));
            for (int i = 0; i < 5; i++){
                if (compareChar("short"[i], stream.get(result_peg_63.getPosition()))){
                    result_peg_63.nextPosition();
                } else {
                    result_peg_63.setPosition(position_peg_65);
                    goto out_peg_66;
                }
            }
                
        }
        goto success_peg_64;
        out_peg_66:
        goto out_peg_67;
        success_peg_64:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_63;
        stream.update(result_peg_63.getPosition());
        
        
        return result_peg_63;
        out_peg_67:
        Result result_peg_68(myposition);
        
        
        {
            int position_peg_70 = result_peg_68.getPosition();
            
            result_peg_68.setValue(Value((void*) "int"));
            for (int i = 0; i < 3; i++){
                if (compareChar("int"[i], stream.get(result_peg_68.getPosition()))){
                    result_peg_68.nextPosition();
                } else {
                    result_peg_68.setPosition(position_peg_70);
                    goto out_peg_71;
                }
            }
                
        }
        goto success_peg_69;
        out_peg_71:
        goto out_peg_72;
        success_peg_69:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_68;
        stream.update(result_peg_68.getPosition());
        
        
        return result_peg_68;
        out_peg_72:
        Result result_peg_73(myposition);
        
        
        {
            int position_peg_75 = result_peg_73.getPosition();
            
            result_peg_73.setValue(Value((void*) "long"));
            for (int i = 0; i < 4; i++){
                if (compareChar("long"[i], stream.get(result_peg_73.getPosition()))){
                    result_peg_73.nextPosition();
                } else {
                    result_peg_73.setPosition(position_peg_75);
                    goto out_peg_76;
                }
            }
                
        }
        goto success_peg_74;
        out_peg_76:
        goto out_peg_77;
        success_peg_74:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_73;
        stream.update(result_peg_73.getPosition());
        
        
        return result_peg_73;
        out_peg_77:
        Result result_peg_78(myposition);
        
        
        {
            int position_peg_80 = result_peg_78.getPosition();
            
            result_peg_78.setValue(Value((void*) "signed"));
            for (int i = 0; i < 6; i++){
                if (compareChar("signed"[i], stream.get(result_peg_78.getPosition()))){
                    result_peg_78.nextPosition();
                } else {
                    result_peg_78.setPosition(position_peg_80);
                    goto out_peg_81;
                }
            }
                
        }
        goto success_peg_79;
        out_peg_81:
        goto out_peg_82;
        success_peg_79:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_78;
        stream.update(result_peg_78.getPosition());
        
        
        return result_peg_78;
        out_peg_82:
        Result result_peg_83(myposition);
        
        
        {
            int position_peg_85 = result_peg_83.getPosition();
            
            result_peg_83.setValue(Value((void*) "unsigned"));
            for (int i = 0; i < 8; i++){
                if (compareChar("unsigned"[i], stream.get(result_peg_83.getPosition()))){
                    result_peg_83.nextPosition();
                } else {
                    result_peg_83.setPosition(position_peg_85);
                    goto out_peg_86;
                }
            }
                
        }
        goto success_peg_84;
        out_peg_86:
        goto out_peg_87;
        success_peg_84:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_83;
        stream.update(result_peg_83.getPosition());
        
        
        return result_peg_83;
        out_peg_87:
        Result result_peg_88(myposition);
        
        
        {
            int position_peg_90 = result_peg_88.getPosition();
            
            result_peg_88.setValue(Value((void*) "void"));
            for (int i = 0; i < 4; i++){
                if (compareChar("void"[i], stream.get(result_peg_88.getPosition()))){
                    result_peg_88.nextPosition();
                } else {
                    result_peg_88.setPosition(position_peg_90);
                    goto out_peg_91;
                }
            }
                
        }
        goto success_peg_89;
        out_peg_91:
        goto out_peg_92;
        success_peg_89:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_88;
        stream.update(result_peg_88.getPosition());
        
        
        return result_peg_88;
        out_peg_92:
        Result result_peg_93(myposition);
        
        
        {
            int position_peg_95 = result_peg_93.getPosition();
            
            result_peg_93.setValue(Value((void*) "include"));
            for (int i = 0; i < 7; i++){
                if (compareChar("include"[i], stream.get(result_peg_93.getPosition()))){
                    result_peg_93.nextPosition();
                } else {
                    result_peg_93.setPosition(position_peg_95);
                    goto out_peg_96;
                }
            }
                
        }
        goto success_peg_94;
        out_peg_96:
        goto out_peg_97;
        success_peg_94:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_93;
        stream.update(result_peg_93.getPosition());
        
        
        return result_peg_93;
        out_peg_97:
        Result result_peg_98(myposition);
        
        
        {
            int position_peg_100 = result_peg_98.getPosition();
            
            result_peg_98.setValue(Value((void*) "array"));
            for (int i = 0; i < 5; i++){
                if (compareChar("array"[i], stream.get(result_peg_98.getPosition()))){
                    result_peg_98.nextPosition();
                } else {
                    result_peg_98.setPosition(position_peg_100);
                    goto out_peg_101;
                }
            }
                
        }
        goto success_peg_99;
        out_peg_101:
        goto out_peg_102;
        success_peg_99:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_98;
        stream.update(result_peg_98.getPosition());
        
        
        return result_peg_98;
        out_peg_102:
        Result result_peg_103(myposition);
        
        
        {
            int position_peg_105 = result_peg_103.getPosition();
            
            result_peg_103.setValue(Value((void*) "null"));
            for (int i = 0; i < 4; i++){
                if (compareChar("null"[i], stream.get(result_peg_103.getPosition()))){
                    result_peg_103.nextPosition();
                } else {
                    result_peg_103.setPosition(position_peg_105);
                    goto out_peg_106;
                }
            }
                
        }
        goto success_peg_104;
        out_peg_106:
        goto out_peg_107;
        success_peg_104:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_103;
        stream.update(result_peg_103.getPosition());
        
        
        return result_peg_103;
        out_peg_107:
        Result result_peg_108(myposition);
        
        
        {
            int position_peg_110 = result_peg_108.getPosition();
            
            result_peg_108.setValue(Value((void*) "this"));
            for (int i = 0; i < 4; i++){
                if (compareChar("this"[i], stream.get(result_peg_108.getPosition()))){
                    result_peg_108.nextPosition();
                } else {
                    result_peg_108.setPosition(position_peg_110);
                    goto out_peg_111;
                }
            }
                
        }
        goto success_peg_109;
        out_peg_111:
        goto out_peg_112;
        success_peg_109:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_108;
        stream.update(result_peg_108.getPosition());
        
        
        return result_peg_108;
        out_peg_112:
        Result result_peg_113(myposition);
        
        
        {
            int position_peg_115 = result_peg_113.getPosition();
            
            result_peg_113.setValue(Value((void*) "in"));
            for (int i = 0; i < 2; i++){
                if (compareChar("in"[i], stream.get(result_peg_113.getPosition()))){
                    result_peg_113.nextPosition();
                } else {
                    result_peg_113.setPosition(position_peg_115);
                    goto out_peg_116;
                }
            }
                
        }
        goto success_peg_114;
        out_peg_116:
        goto out_peg_117;
        success_peg_114:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_113;
        stream.update(result_peg_113.getPosition());
        
        
        return result_peg_113;
        out_peg_117:
        Result result_peg_118(myposition);
        
        
        {
            int position_peg_120 = result_peg_118.getPosition();
            
            result_peg_118.setValue(Value((void*) "out"));
            for (int i = 0; i < 3; i++){
                if (compareChar("out"[i], stream.get(result_peg_118.getPosition()))){
                    result_peg_118.nextPosition();
                } else {
                    result_peg_118.setPosition(position_peg_120);
                    goto out_peg_121;
                }
            }
                
        }
        goto success_peg_119;
        out_peg_121:
        goto out_peg_122;
        success_peg_119:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_118;
        stream.update(result_peg_118.getPosition());
        
        
        return result_peg_118;
        out_peg_122:
        Result result_peg_123(myposition);
        
        
        {
            int position_peg_125 = result_peg_123.getPosition();
            
            result_peg_123.setValue(Value((void*) "alloc"));
            for (int i = 0; i < 5; i++){
                if (compareChar("alloc"[i], stream.get(result_peg_123.getPosition()))){
                    result_peg_123.nextPosition();
                } else {
                    result_peg_123.setPosition(position_peg_125);
                    goto out_peg_126;
                }
            }
                
        }
        goto success_peg_124;
        out_peg_126:
        goto out_peg_127;
        success_peg_124:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_123;
        stream.update(result_peg_123.getPosition());
        
        
        return result_peg_123;
        out_peg_127:
        Result result_peg_128(myposition);
        
        
        {
            int position_peg_130 = result_peg_128.getPosition();
            
            result_peg_128.setValue(Value((void*) "dealloc"));
            for (int i = 0; i < 7; i++){
                if (compareChar("dealloc"[i], stream.get(result_peg_128.getPosition()))){
                    result_peg_128.nextPosition();
                } else {
                    result_peg_128.setPosition(position_peg_130);
                    goto out_peg_131;
                }
            }
                
        }
        goto success_peg_129;
        out_peg_131:
        goto out_peg_132;
        success_peg_129:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_128;
        stream.update(result_peg_128.getPosition());
        
        
        return result_peg_128;
        out_peg_132:
        Result result_peg_133(myposition);
        
        
        {
            int position_peg_135 = result_peg_133.getPosition();
            
            result_peg_133.setValue(Value((void*) "bind"));
            for (int i = 0; i < 4; i++){
                if (compareChar("bind"[i], stream.get(result_peg_133.getPosition()))){
                    result_peg_133.nextPosition();
                } else {
                    result_peg_133.setPosition(position_peg_135);
                    goto out_peg_136;
                }
            }
                
        }
        goto success_peg_134;
        out_peg_136:
        goto out_peg_137;
        success_peg_134:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_133;
        stream.update(result_peg_133.getPosition());
        
        
        return result_peg_133;
        out_peg_137:
        Result result_peg_138(myposition);
        
        
        {
            int position_peg_140 = result_peg_138.getPosition();
            
            result_peg_138.setValue(Value((void*) "caller"));
            for (int i = 0; i < 6; i++){
                if (compareChar("caller"[i], stream.get(result_peg_138.getPosition()))){
                    result_peg_138.nextPosition();
                } else {
                    result_peg_138.setPosition(position_peg_140);
                    goto out_peg_141;
                }
            }
                
        }
        goto success_peg_139;
        out_peg_141:
        goto out_peg_142;
        success_peg_139:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_138;
        stream.update(result_peg_138.getPosition());
        
        
        return result_peg_138;
        out_peg_142:
        Result result_peg_143(myposition);
        
        
        {
            int position_peg_145 = result_peg_143.getPosition();
            
            result_peg_143.setValue(Value((void*) "callee"));
            for (int i = 0; i < 6; i++){
                if (compareChar("callee"[i], stream.get(result_peg_143.getPosition()))){
                    result_peg_143.nextPosition();
                } else {
                    result_peg_143.setPosition(position_peg_145);
                    goto out_peg_146;
                }
            }
                
        }
        goto success_peg_144;
        out_peg_146:
        goto out_peg_147;
        success_peg_144:
        ;
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = result_peg_143;
        stream.update(result_peg_143.getPosition());
        
        
        return result_peg_143;
        out_peg_147:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_lex_keyword = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_file(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_file.calculated()){
        return column_peg_2.chunk2->chunk_file;
    }
    
    
    RuleTrace trace_peg_1(stream, "file");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_driver_file(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_file = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_module_file(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_file = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_file = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_driver_file(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_driver_file.calculated()){
        return column_peg_2.chunk2->chunk_driver_file;
    }
    
    
    RuleTrace trace_peg_1(stream, "driver_file");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    int save_peg_7 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_7);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_include_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_9 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_9);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_driver_def(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_11;
                }
            
            
            
            int save_peg_13 = result_peg_3.getPosition();
                {
                
                    int save_peg_15 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_15);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_include_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_13);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_17 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_17);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            if ('\0' == stream.get(result_peg_3.getPosition())){
                    result_peg_3.nextPosition();
                    result_peg_3.setValue(Value((void *) '\0'));
                } else {
                    goto out_peg_11;
                }
            
            
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_driver_file = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_11:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_driver_file = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_include_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_include_list.calculated()){
        return column_peg_2.chunk3->chunk_include_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "include_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_include_stmt(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_include_stmt(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_include_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_include_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_include_stmt(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_include_stmt.calculated()){
        return column_peg_2.chunk3->chunk_include_stmt;
    }
    
    
    RuleTrace trace_peg_1(stream, "include_stmt");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "include"));
                    for (int i = 0; i < 7; i++){
                        if (compareChar("include"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_10 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_10);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_tok_path(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_13 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_13);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_include_stmt = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_include_stmt = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_driver_def(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_driver_def.calculated()){
        return column_peg_2.chunk3->chunk_driver_def;
    }
    
    
    RuleTrace trace_peg_1(stream, "driver_def");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "driver"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("driver"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_12 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_12);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "{"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("{"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
            
            int save_peg_18 = result_peg_3.getPosition();
                {
                
                    int save_peg_20 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_20);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_import_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_18);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_22 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_22);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_24 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "}"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("}"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_24);
                            goto out_peg_25;
                        }
                    }
                        
                }
                goto success_peg_23;
                out_peg_25:
                goto out_peg_8;
                success_peg_23:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_driver_def = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_driver_def = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_import_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_import_list.calculated()){
        return column_peg_2.chunk3->chunk_import_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "import_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_import_stmt(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_import_stmt(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_import_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_import_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_import_stmt(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_import_stmt.calculated()){
        return column_peg_2.chunk3->chunk_import_stmt;
    }
    
    
    RuleTrace trace_peg_1(stream, "import_stmt");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "import"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("import"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_12 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_12);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_14 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_14);
                            goto out_peg_15;
                        }
                    }
                        
                }
                goto success_peg_13;
                out_peg_15:
                goto out_peg_8;
                success_peg_13:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_import_stmt = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_import_stmt = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_module_file(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_module_file.calculated()){
        return column_peg_2.chunk4->chunk_module_file;
    }
    
    
    RuleTrace trace_peg_1(stream, "module_file");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_5);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_module_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_7;
                }
            
            
            
            int save_peg_9 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_9);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            if ('\0' == stream.get(result_peg_3.getPosition())){
                    result_peg_3.nextPosition();
                    result_peg_3.setValue(Value((void *) '\0'));
                } else {
                    goto out_peg_7;
                }
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_file = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_file = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_module_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_module_list.calculated()){
        return column_peg_2.chunk4->chunk_module_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "module_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_module_def(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_module_def(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_module_def(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_module_def.calculated()){
        return column_peg_2.chunk4->chunk_module_def;
    }
    
    
    RuleTrace trace_peg_1(stream, "module_def");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "module"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("module"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_12 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_12);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "{"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("{"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
            
            int save_peg_18 = result_peg_3.getPosition();
                {
                
                    int save_peg_20 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_20);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_module_item_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_18);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_22 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_22);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_24 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "}"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("}"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_24);
                            goto out_peg_25;
                        }
                    }
                        
                }
                goto success_peg_23;
                out_peg_25:
                goto out_peg_8;
                success_peg_23:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_def = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_def = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_module_item_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_module_item_list.calculated()){
        return column_peg_2.chunk4->chunk_module_item_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "module_item_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_module_item(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_module_item(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_module_item(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_module_item.calculated()){
        return column_peg_2.chunk4->chunk_module_item;
    }
    
    
    RuleTrace trace_peg_1(stream, "module_item");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_header_stmt(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_proj_def(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
        Result result_peg_7(myposition);
        
        
        result_peg_7 = rule_rpc_def(stream, result_peg_7.getPosition());
        if (result_peg_7.error()){
            goto out_peg_8;
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item = result_peg_7;
        stream.update(result_peg_7.getPosition());
        
        
        return result_peg_7;
        out_peg_8:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_module_item = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_header_stmt(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_header_stmt.calculated()){
        return column_peg_2.chunk5->chunk_header_stmt;
    }
    
    
    RuleTrace trace_peg_1(stream, "header_stmt");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "header"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("header"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_10 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_10);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_tok_path(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_13 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_13);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_header_stmt = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
        Result result_peg_17(myposition);
        
        
        {
        
            {
                    int position_peg_20 = result_peg_17.getPosition();
                    
                    result_peg_17.setValue(Value((void*) "header"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("header"[i], stream.get(result_peg_17.getPosition()))){
                            result_peg_17.nextPosition();
                        } else {
                            result_peg_17.setPosition(position_peg_20);
                            goto out_peg_21;
                        }
                    }
                        
                }
                goto success_peg_19;
                out_peg_21:
                goto out_peg_22;
                success_peg_19:
                ;
            
            
            
            int save_peg_24 = result_peg_17.getPosition();
                
                result_peg_17 = rule_tok_space(stream, result_peg_17.getPosition());
                if (result_peg_17.error()){
                    
                    result_peg_17 = Result(save_peg_24);
                    result_peg_17.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_17 = rule_tok_string(stream, result_peg_17.getPosition());
                if (result_peg_17.error()){
                    goto out_peg_22;
                }
            
            
            
            int save_peg_27 = result_peg_17.getPosition();
                
                result_peg_17 = rule_tok_space(stream, result_peg_17.getPosition());
                if (result_peg_17.error()){
                    
                    result_peg_17 = Result(save_peg_27);
                    result_peg_17.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_29 = result_peg_17.getPosition();
                    
                    result_peg_17.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_17.getPosition()))){
                            result_peg_17.nextPosition();
                        } else {
                            result_peg_17.setPosition(position_peg_29);
                            goto out_peg_30;
                        }
                    }
                        
                }
                goto success_peg_28;
                out_peg_30:
                goto out_peg_22;
                success_peg_28:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_header_stmt = result_peg_17;
        stream.update(result_peg_17.getPosition());
        
        
        return result_peg_17;
        out_peg_22:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_header_stmt = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_def(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_def.calculated()){
        return column_peg_2.chunk5->chunk_proj_def;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_def");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "projection"));
                    for (int i = 0; i < 10; i++){
                        if (compareChar("projection"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_10 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_10);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_13 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "<"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("<"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_13);
                            goto out_peg_14;
                        }
                    }
                        
                }
                goto success_peg_12;
                out_peg_14:
                goto out_peg_8;
                success_peg_12:
                ;
            
            
            
            int save_peg_16 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_16);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_proj_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_19 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_19);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_22 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ">"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(">"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_22);
                            goto out_peg_23;
                        }
                    }
                        
                }
                goto success_peg_21;
                out_peg_23:
                goto out_peg_8;
                success_peg_21:
                ;
            
            
            
            int save_peg_25 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_25);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_28 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_28);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_31 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "{"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("{"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_31);
                            goto out_peg_32;
                        }
                    }
                        
                }
                goto success_peg_30;
                out_peg_32:
                goto out_peg_8;
                success_peg_30:
                ;
            
            
            
            int save_peg_34 = result_peg_3.getPosition();
                {
                
                    int save_peg_36 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_36);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_proj_field_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_34);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_38 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_38);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_40 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "}"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("}"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_40);
                            goto out_peg_41;
                        }
                    }
                        
                }
                goto success_peg_39;
                out_peg_41:
                goto out_peg_8;
                success_peg_39:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_def = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_def = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_type.calculated()){
        return column_peg_2.chunk5->chunk_proj_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_proj_struct_type(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_proj_union_type(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_type = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_struct_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_struct_type.calculated()){
        return column_peg_2.chunk5->chunk_proj_struct_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_struct_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "struct"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("struct"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_struct_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_struct_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_union_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_union_type.calculated()){
        return column_peg_2.chunk5->chunk_proj_union_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_union_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "union"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("union"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_12 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_12);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ","));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(","[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
            
            int save_peg_18 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_18);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_field_ref(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_union_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_union_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_field_ref(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk6 != 0 && column_peg_2.chunk6->chunk_field_ref.calculated()){
        return column_peg_2.chunk6->chunk_field_ref;
    }
    
    
    RuleTrace trace_peg_1(stream, "field_ref");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_field_rel_ref(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_ref = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_field_abs_ref(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_ref = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
    
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_ref = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_field_rel_ref(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk6 != 0 && column_peg_2.chunk6->chunk_field_rel_ref.calculated()){
        return column_peg_2.chunk6->chunk_field_rel_ref;
    }
    
    
    RuleTrace trace_peg_1(stream, "field_rel_ref");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        {
                                int position_peg_10 = result_peg_7.getPosition();
                                
                                result_peg_7.setValue(Value((void*) "->"));
                                for (int i = 0; i < 2; i++){
                                    if (compareChar("->"[i], stream.get(result_peg_7.getPosition()))){
                                        result_peg_7.nextPosition();
                                    } else {
                                        result_peg_7.setPosition(position_peg_10);
                                        goto out_peg_11;
                                    }
                                }
                                    
                            }
                            goto success_peg_9;
                            out_peg_11:
                            goto loop_peg_6;
                            success_peg_9:
                            ;
                        
                        
                        
                        result_peg_7 = rule_tok_ident(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_rel_ref = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_rel_ref = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_field_abs_ref(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk6 != 0 && column_peg_2.chunk6->chunk_field_abs_ref.calculated()){
        return column_peg_2.chunk6->chunk_field_abs_ref;
    }
    
    
    RuleTrace trace_peg_1(stream, "field_abs_ref");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "this"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("this"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_9 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_12 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "->"));
                            for (int i = 0; i < 2; i++){
                                if (compareChar("->"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_12);
                                    goto out_peg_13;
                                }
                            }
                                
                        }
                        goto success_peg_11;
                        out_peg_13:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_11:
                        ;
                    
                    
                    
                    result_peg_3 = rule_field_rel_ref(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_9);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_abs_ref = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_field_abs_ref = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_field_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk6 != 0 && column_peg_2.chunk6->chunk_proj_field_list.calculated()){
        return column_peg_2.chunk6->chunk_proj_field_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_field_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_proj_field(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_proj_field(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_proj_field_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_proj_field_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_field(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk6 != 0 && column_peg_2.chunk6->chunk_proj_field.calculated()){
        return column_peg_2.chunk6->chunk_proj_field;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_field");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_var_decl(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_7 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_7);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_9 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_9);
                            goto out_peg_10;
                        }
                    }
                        
                }
                goto success_peg_8;
                out_peg_10:
                goto out_peg_5;
                success_peg_8:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_proj_field = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk6 == 0){
            column_peg_2.chunk6 = new Chunk6();
        }
        column_peg_2.chunk6->chunk_proj_field = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_ptr(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk7 != 0 && column_peg_2.chunk7->chunk_typename_ptr.calculated()){
        return column_peg_2.chunk7->chunk_typename_ptr;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_ptr");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_typename_any_no_ptr(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_7 = result_peg_3.getPosition();
                {
                
                    int save_peg_9 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_9);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_val_attrs(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_7);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                    
                        int save_peg_13 = result_peg_11.getPosition();
                            {
                            
                                int save_peg_15 = result_peg_11.getPosition();
                                    
                                    result_peg_11 = rule_tok_space(stream, result_peg_11.getPosition());
                                    if (result_peg_11.error()){
                                        
                                        result_peg_11 = Result(save_peg_15);
                                        result_peg_11.setValue(Value((void*) 0));
                                        
                                    }
                                
                                
                                
                                result_peg_11 = rule_ptr_attrs(stream, result_peg_11.getPosition());
                                    if (result_peg_11.error()){
                                        
                                        result_peg_11 = Result(save_peg_13);
                                        result_peg_11.setValue(Value((void*) 0));
                                        
                                    }
                                
                                
                            }
                        
                        
                        
                        int save_peg_17 = result_peg_11.getPosition();
                            
                            result_peg_11 = rule_tok_space(stream, result_peg_11.getPosition());
                            if (result_peg_11.error()){
                                
                                result_peg_11 = Result(save_peg_17);
                                result_peg_11.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        {
                                int position_peg_20 = result_peg_11.getPosition();
                                
                                result_peg_11.setValue(Value((void*) "*"));
                                for (int i = 0; i < 1; i++){
                                    if (compareChar("*"[i], stream.get(result_peg_11.getPosition()))){
                                        result_peg_11.nextPosition();
                                    } else {
                                        result_peg_11.setPosition(position_peg_20);
                                        goto out_peg_21;
                                    }
                                }
                                    
                            }
                            goto success_peg_19;
                            out_peg_21:
                            goto loop_peg_10;
                            success_peg_19:
                            ;
                        
                        
                        
                        int save_peg_22 = result_peg_11.getPosition();
                            {
                            
                                int save_peg_24 = result_peg_11.getPosition();
                                    
                                    result_peg_11 = rule_tok_space(stream, result_peg_11.getPosition());
                                    if (result_peg_11.error()){
                                        
                                        result_peg_11 = Result(save_peg_24);
                                        result_peg_11.setValue(Value((void*) 0));
                                        
                                    }
                                
                                
                                
                                {
                                        int position_peg_26 = result_peg_11.getPosition();
                                        
                                        result_peg_11.setValue(Value((void*) "const"));
                                        for (int i = 0; i < 5; i++){
                                            if (compareChar("const"[i], stream.get(result_peg_11.getPosition()))){
                                                result_peg_11.nextPosition();
                                            } else {
                                                result_peg_11.setPosition(position_peg_26);
                                                goto out_peg_27;
                                            }
                                        }
                                            
                                    }
                                    goto success_peg_25;
                                    out_peg_27:
                                    
                                    result_peg_11 = Result(save_peg_22);
                                    result_peg_11.setValue(Value((void*) 0));
                                    
                                    success_peg_25:
                                    ;
                                
                                
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_typename_ptr = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_typename_ptr = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_val_attr(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk7 != 0 && column_peg_2.chunk7->chunk_val_attr.calculated()){
        return column_peg_2.chunk7->chunk_val_attr;
    }
    
    
    RuleTrace trace_peg_1(stream, "val_attr");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "in"));
            for (int i = 0; i < 2; i++){
                if (compareChar("in"[i], stream.get(result_peg_3.getPosition()))){
                    result_peg_3.nextPosition();
                } else {
                    result_peg_3.setPosition(position_peg_5);
                    goto out_peg_6;
                }
            }
                
        }
        goto success_peg_4;
        out_peg_6:
        goto out_peg_7;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_val_attr = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "out"));
            for (int i = 0; i < 3; i++){
                if (compareChar("out"[i], stream.get(result_peg_8.getPosition()))){
                    result_peg_8.nextPosition();
                } else {
                    result_peg_8.setPosition(position_peg_10);
                    goto out_peg_11;
                }
            }
                
        }
        goto success_peg_9;
        out_peg_11:
        goto out_peg_12;
        success_peg_9:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_val_attr = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
    
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_val_attr = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_rpc_side_spec(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk7 != 0 && column_peg_2.chunk7->chunk_rpc_side_spec.calculated()){
        return column_peg_2.chunk7->chunk_rpc_side_spec;
    }
    
    
    RuleTrace trace_peg_1(stream, "rpc_side_spec");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "caller"));
            for (int i = 0; i < 6; i++){
                if (compareChar("caller"[i], stream.get(result_peg_3.getPosition()))){
                    result_peg_3.nextPosition();
                } else {
                    result_peg_3.setPosition(position_peg_5);
                    goto out_peg_6;
                }
            }
                
        }
        goto success_peg_4;
        out_peg_6:
        goto out_peg_7;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_rpc_side_spec = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "callee"));
            for (int i = 0; i < 6; i++){
                if (compareChar("callee"[i], stream.get(result_peg_8.getPosition()))){
                    result_peg_8.nextPosition();
                } else {
                    result_peg_8.setPosition(position_peg_10);
                    goto out_peg_11;
                }
            }
                
        }
        goto success_peg_9;
        out_peg_11:
        goto out_peg_12;
        success_peg_9:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_rpc_side_spec = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
    
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_rpc_side_spec = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_ptr_attr(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk7 != 0 && column_peg_2.chunk7->chunk_ptr_attr.calculated()){
        return column_peg_2.chunk7->chunk_ptr_attr;
    }
    
    
    RuleTrace trace_peg_1(stream, "ptr_attr");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "in"));
            for (int i = 0; i < 2; i++){
                if (compareChar("in"[i], stream.get(result_peg_3.getPosition()))){
                    result_peg_3.nextPosition();
                } else {
                    result_peg_3.setPosition(position_peg_5);
                    goto out_peg_6;
                }
            }
                
        }
        goto success_peg_4;
        out_peg_6:
        goto out_peg_7;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "out"));
            for (int i = 0; i < 3; i++){
                if (compareChar("out"[i], stream.get(result_peg_8.getPosition()))){
                    result_peg_8.nextPosition();
                } else {
                    result_peg_8.setPosition(position_peg_10);
                    goto out_peg_11;
                }
            }
                
        }
        goto success_peg_9;
        out_peg_11:
        goto out_peg_12;
        success_peg_9:
        ;
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
        Result result_peg_13(myposition);
        
        
        {
        
            {
                    int position_peg_16 = result_peg_13.getPosition();
                    
                    result_peg_13.setValue(Value((void*) "alloc"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("alloc"[i], stream.get(result_peg_13.getPosition()))){
                            result_peg_13.nextPosition();
                        } else {
                            result_peg_13.setPosition(position_peg_16);
                            goto out_peg_17;
                        }
                    }
                        
                }
                goto success_peg_15;
                out_peg_17:
                goto out_peg_18;
                success_peg_15:
                ;
            
            
            
            int save_peg_19 = result_peg_13.getPosition();
                {
                
                    int save_peg_21 = result_peg_13.getPosition();
                        
                        result_peg_13 = rule_tok_space(stream, result_peg_13.getPosition());
                        if (result_peg_13.error()){
                            
                            result_peg_13 = Result(save_peg_21);
                            result_peg_13.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_24 = result_peg_13.getPosition();
                            
                            result_peg_13.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_13.getPosition()))){
                                    result_peg_13.nextPosition();
                                } else {
                                    result_peg_13.setPosition(position_peg_24);
                                    goto out_peg_25;
                                }
                            }
                                
                        }
                        goto success_peg_23;
                        out_peg_25:
                        
                        result_peg_13 = Result(save_peg_19);
                        result_peg_13.setValue(Value((void*) 0));
                        
                        success_peg_23:
                        ;
                    
                    
                    
                    int save_peg_27 = result_peg_13.getPosition();
                        
                        result_peg_13 = rule_tok_space(stream, result_peg_13.getPosition());
                        if (result_peg_13.error()){
                            
                            result_peg_13 = Result(save_peg_27);
                            result_peg_13.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_13 = rule_rpc_side_spec(stream, result_peg_13.getPosition());
                        if (result_peg_13.error()){
                            
                            result_peg_13 = Result(save_peg_19);
                            result_peg_13.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    int save_peg_30 = result_peg_13.getPosition();
                        
                        result_peg_13 = rule_tok_space(stream, result_peg_13.getPosition());
                        if (result_peg_13.error()){
                            
                            result_peg_13 = Result(save_peg_30);
                            result_peg_13.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_32 = result_peg_13.getPosition();
                            
                            result_peg_13.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_13.getPosition()))){
                                    result_peg_13.nextPosition();
                                } else {
                                    result_peg_13.setPosition(position_peg_32);
                                    goto out_peg_33;
                                }
                            }
                                
                        }
                        goto success_peg_31;
                        out_peg_33:
                        
                        result_peg_13 = Result(save_peg_19);
                        result_peg_13.setValue(Value((void*) 0));
                        
                        success_peg_31:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = result_peg_13;
        stream.update(result_peg_13.getPosition());
        
        
        return result_peg_13;
        out_peg_18:
        Result result_peg_34(myposition);
        
        
        {
        
            {
                    int position_peg_37 = result_peg_34.getPosition();
                    
                    result_peg_34.setValue(Value((void*) "dealloc"));
                    for (int i = 0; i < 7; i++){
                        if (compareChar("dealloc"[i], stream.get(result_peg_34.getPosition()))){
                            result_peg_34.nextPosition();
                        } else {
                            result_peg_34.setPosition(position_peg_37);
                            goto out_peg_38;
                        }
                    }
                        
                }
                goto success_peg_36;
                out_peg_38:
                goto out_peg_39;
                success_peg_36:
                ;
            
            
            
            int save_peg_40 = result_peg_34.getPosition();
                {
                
                    int save_peg_42 = result_peg_34.getPosition();
                        
                        result_peg_34 = rule_tok_space(stream, result_peg_34.getPosition());
                        if (result_peg_34.error()){
                            
                            result_peg_34 = Result(save_peg_42);
                            result_peg_34.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_45 = result_peg_34.getPosition();
                            
                            result_peg_34.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_34.getPosition()))){
                                    result_peg_34.nextPosition();
                                } else {
                                    result_peg_34.setPosition(position_peg_45);
                                    goto out_peg_46;
                                }
                            }
                                
                        }
                        goto success_peg_44;
                        out_peg_46:
                        
                        result_peg_34 = Result(save_peg_40);
                        result_peg_34.setValue(Value((void*) 0));
                        
                        success_peg_44:
                        ;
                    
                    
                    
                    int save_peg_48 = result_peg_34.getPosition();
                        
                        result_peg_34 = rule_tok_space(stream, result_peg_34.getPosition());
                        if (result_peg_34.error()){
                            
                            result_peg_34 = Result(save_peg_48);
                            result_peg_34.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_34 = rule_rpc_side_spec(stream, result_peg_34.getPosition());
                        if (result_peg_34.error()){
                            
                            result_peg_34 = Result(save_peg_40);
                            result_peg_34.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    int save_peg_51 = result_peg_34.getPosition();
                        
                        result_peg_34 = rule_tok_space(stream, result_peg_34.getPosition());
                        if (result_peg_34.error()){
                            
                            result_peg_34 = Result(save_peg_51);
                            result_peg_34.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_53 = result_peg_34.getPosition();
                            
                            result_peg_34.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_34.getPosition()))){
                                    result_peg_34.nextPosition();
                                } else {
                                    result_peg_34.setPosition(position_peg_53);
                                    goto out_peg_54;
                                }
                            }
                                
                        }
                        goto success_peg_52;
                        out_peg_54:
                        
                        result_peg_34 = Result(save_peg_40);
                        result_peg_34.setValue(Value((void*) 0));
                        
                        success_peg_52:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = result_peg_34;
        stream.update(result_peg_34.getPosition());
        
        
        return result_peg_34;
        out_peg_39:
        Result result_peg_55(myposition);
        
        
        {
        
            {
                    int position_peg_58 = result_peg_55.getPosition();
                    
                    result_peg_55.setValue(Value((void*) "bind"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("bind"[i], stream.get(result_peg_55.getPosition()))){
                            result_peg_55.nextPosition();
                        } else {
                            result_peg_55.setPosition(position_peg_58);
                            goto out_peg_59;
                        }
                    }
                        
                }
                goto success_peg_57;
                out_peg_59:
                goto out_peg_60;
                success_peg_57:
                ;
            
            
            
            int save_peg_61 = result_peg_55.getPosition();
                {
                
                    int save_peg_63 = result_peg_55.getPosition();
                        
                        result_peg_55 = rule_tok_space(stream, result_peg_55.getPosition());
                        if (result_peg_55.error()){
                            
                            result_peg_55 = Result(save_peg_63);
                            result_peg_55.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_66 = result_peg_55.getPosition();
                            
                            result_peg_55.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_55.getPosition()))){
                                    result_peg_55.nextPosition();
                                } else {
                                    result_peg_55.setPosition(position_peg_66);
                                    goto out_peg_67;
                                }
                            }
                                
                        }
                        goto success_peg_65;
                        out_peg_67:
                        
                        result_peg_55 = Result(save_peg_61);
                        result_peg_55.setValue(Value((void*) 0));
                        
                        success_peg_65:
                        ;
                    
                    
                    
                    int save_peg_69 = result_peg_55.getPosition();
                        
                        result_peg_55 = rule_tok_space(stream, result_peg_55.getPosition());
                        if (result_peg_55.error()){
                            
                            result_peg_55 = Result(save_peg_69);
                            result_peg_55.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_55 = rule_rpc_side_spec(stream, result_peg_55.getPosition());
                        if (result_peg_55.error()){
                            
                            result_peg_55 = Result(save_peg_61);
                            result_peg_55.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    int save_peg_72 = result_peg_55.getPosition();
                        
                        result_peg_55 = rule_tok_space(stream, result_peg_55.getPosition());
                        if (result_peg_55.error()){
                            
                            result_peg_55 = Result(save_peg_72);
                            result_peg_55.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    {
                            int position_peg_74 = result_peg_55.getPosition();
                            
                            result_peg_55.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_55.getPosition()))){
                                    result_peg_55.nextPosition();
                                } else {
                                    result_peg_55.setPosition(position_peg_74);
                                    goto out_peg_75;
                                }
                            }
                                
                        }
                        goto success_peg_73;
                        out_peg_75:
                        
                        result_peg_55 = Result(save_peg_61);
                        result_peg_55.setValue(Value((void*) 0));
                        
                        success_peg_73:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = result_peg_55;
        stream.update(result_peg_55.getPosition());
        
        
        return result_peg_55;
        out_peg_60:
    
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_ptr_attr = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_val_attrs(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk7 != 0 && column_peg_2.chunk7->chunk_val_attrs.calculated()){
        return column_peg_2.chunk7->chunk_val_attrs;
    }
    
    
    RuleTrace trace_peg_1(stream, "val_attrs");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "["));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("["[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_10 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_10);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_val_attr_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_13 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_13);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "]"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("]"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_val_attrs = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk7 == 0){
            column_peg_2.chunk7 = new Chunk7();
        }
        column_peg_2.chunk7->chunk_val_attrs = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_ptr_attrs(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk8 != 0 && column_peg_2.chunk8->chunk_ptr_attrs.calculated()){
        return column_peg_2.chunk8->chunk_ptr_attrs;
    }
    
    
    RuleTrace trace_peg_1(stream, "ptr_attrs");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "["));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("["[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            int save_peg_10 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_10);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_ptr_attr_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_13 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_13);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "]"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("]"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_ptr_attrs = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_ptr_attrs = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_val_attr_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk8 != 0 && column_peg_2.chunk8->chunk_val_attr_list.calculated()){
        return column_peg_2.chunk8->chunk_val_attr_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "val_attr_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_val_attr(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        {
                                int position_peg_12 = result_peg_7.getPosition();
                                
                                result_peg_7.setValue(Value((void*) ","));
                                for (int i = 0; i < 1; i++){
                                    if (compareChar(","[i], stream.get(result_peg_7.getPosition()))){
                                        result_peg_7.nextPosition();
                                    } else {
                                        result_peg_7.setPosition(position_peg_12);
                                        goto out_peg_13;
                                    }
                                }
                                    
                            }
                            goto success_peg_11;
                            out_peg_13:
                            goto loop_peg_6;
                            success_peg_11:
                            ;
                        
                        
                        
                        int save_peg_15 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_15);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_val_attr(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_val_attr_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_val_attr_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_ptr_attr_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk8 != 0 && column_peg_2.chunk8->chunk_ptr_attr_list.calculated()){
        return column_peg_2.chunk8->chunk_ptr_attr_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "ptr_attr_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_ptr_attr(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        {
                                int position_peg_12 = result_peg_7.getPosition();
                                
                                result_peg_7.setValue(Value((void*) ","));
                                for (int i = 0; i < 1; i++){
                                    if (compareChar(","[i], stream.get(result_peg_7.getPosition()))){
                                        result_peg_7.nextPosition();
                                    } else {
                                        result_peg_7.setPosition(position_peg_12);
                                        goto out_peg_13;
                                    }
                                }
                                    
                            }
                            goto success_peg_11;
                            out_peg_13:
                            goto loop_peg_6;
                            success_peg_11:
                            ;
                        
                        
                        
                        int save_peg_15 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_15);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_ptr_attr(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_ptr_attr_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_ptr_attr_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_var_decl(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk8 != 0 && column_peg_2.chunk8->chunk_var_decl.calculated()){
        return column_peg_2.chunk8->chunk_var_decl;
    }
    
    
    RuleTrace trace_peg_1(stream, "var_decl");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_typename_string(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
        Result result_peg_7(myposition);
        
        
        {
        
            result_peg_7 = rule_typename_arith(stream, result_peg_7.getPosition());
                if (result_peg_7.error()){
                    goto out_peg_9;
                }
            
            
            
            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                if (result_peg_7.error()){
                    goto out_peg_9;
                }
            
            
            
            result_peg_7 = rule_tok_ident(stream, result_peg_7.getPosition());
                if (result_peg_7.error()){
                    goto out_peg_9;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_7;
        stream.update(result_peg_7.getPosition());
        
        
        return result_peg_7;
        out_peg_9:
        Result result_peg_11(myposition);
        
        
        {
        
            result_peg_11 = rule_typename_rpc(stream, result_peg_11.getPosition());
                if (result_peg_11.error()){
                    goto out_peg_13;
                }
            
            
            
            result_peg_11 = rule_tok_space(stream, result_peg_11.getPosition());
                if (result_peg_11.error()){
                    goto out_peg_13;
                }
            
            
            
            result_peg_11 = rule_tok_ident(stream, result_peg_11.getPosition());
                if (result_peg_11.error()){
                    goto out_peg_13;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_11;
        stream.update(result_peg_11.getPosition());
        
        
        return result_peg_11;
        out_peg_13:
        Result result_peg_15(myposition);
        
        
        {
        
            result_peg_15 = rule_typename_proj(stream, result_peg_15.getPosition());
                if (result_peg_15.error()){
                    goto out_peg_17;
                }
            
            
            
            result_peg_15 = rule_tok_space(stream, result_peg_15.getPosition());
                if (result_peg_15.error()){
                    goto out_peg_17;
                }
            
            
            
            result_peg_15 = rule_tok_ident(stream, result_peg_15.getPosition());
                if (result_peg_15.error()){
                    goto out_peg_17;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_15;
        stream.update(result_peg_15.getPosition());
        
        
        return result_peg_15;
        out_peg_17:
        Result result_peg_19(myposition);
        
        
        {
        
            result_peg_19 = rule_typename_array(stream, result_peg_19.getPosition());
                if (result_peg_19.error()){
                    goto out_peg_21;
                }
            
            
            
            int save_peg_23 = result_peg_19.getPosition();
                
                result_peg_19 = rule_tok_space(stream, result_peg_19.getPosition());
                if (result_peg_19.error()){
                    
                    result_peg_19 = Result(save_peg_23);
                    result_peg_19.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_19 = rule_tok_ident(stream, result_peg_19.getPosition());
                if (result_peg_19.error()){
                    goto out_peg_21;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_19;
        stream.update(result_peg_19.getPosition());
        
        
        return result_peg_19;
        out_peg_21:
        Result result_peg_24(myposition);
        
        
        {
        
            result_peg_24 = rule_typename_ptr(stream, result_peg_24.getPosition());
                if (result_peg_24.error()){
                    goto out_peg_26;
                }
            
            
            
            int save_peg_28 = result_peg_24.getPosition();
                
                result_peg_24 = rule_tok_space(stream, result_peg_24.getPosition());
                if (result_peg_24.error()){
                    
                    result_peg_24 = Result(save_peg_28);
                    result_peg_24.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_24 = rule_tok_ident(stream, result_peg_24.getPosition());
                if (result_peg_24.error()){
                    goto out_peg_26;
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = result_peg_24;
        stream.update(result_peg_24.getPosition());
        
        
        return result_peg_24;
        out_peg_26:
    
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_var_decl = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_any_no_ptr(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk8 != 0 && column_peg_2.chunk8->chunk_typename_any_no_ptr.calculated()){
        return column_peg_2.chunk8->chunk_typename_any_no_ptr;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_any_no_ptr");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_typename_arith(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_6 = result_peg_3.getPosition();
                {
                
                    int save_peg_8 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_8);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_val_attrs(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_6);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
        Result result_peg_9(myposition);
        
        
        {
        
            result_peg_9 = rule_typename_array(stream, result_peg_9.getPosition());
                if (result_peg_9.error()){
                    goto out_peg_11;
                }
            
            
            
            int save_peg_12 = result_peg_9.getPosition();
                {
                
                    int save_peg_14 = result_peg_9.getPosition();
                        
                        result_peg_9 = rule_tok_space(stream, result_peg_9.getPosition());
                        if (result_peg_9.error()){
                            
                            result_peg_9 = Result(save_peg_14);
                            result_peg_9.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_9 = rule_val_attrs(stream, result_peg_9.getPosition());
                        if (result_peg_9.error()){
                            
                            result_peg_9 = Result(save_peg_12);
                            result_peg_9.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = result_peg_9;
        stream.update(result_peg_9.getPosition());
        
        
        return result_peg_9;
        out_peg_11:
        Result result_peg_15(myposition);
        
        
        {
        
            result_peg_15 = rule_typename_string(stream, result_peg_15.getPosition());
                if (result_peg_15.error()){
                    goto out_peg_17;
                }
            
            
            
            int save_peg_18 = result_peg_15.getPosition();
                {
                
                    int save_peg_20 = result_peg_15.getPosition();
                        
                        result_peg_15 = rule_tok_space(stream, result_peg_15.getPosition());
                        if (result_peg_15.error()){
                            
                            result_peg_15 = Result(save_peg_20);
                            result_peg_15.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_15 = rule_val_attrs(stream, result_peg_15.getPosition());
                        if (result_peg_15.error()){
                            
                            result_peg_15 = Result(save_peg_18);
                            result_peg_15.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = result_peg_15;
        stream.update(result_peg_15.getPosition());
        
        
        return result_peg_15;
        out_peg_17:
        Result result_peg_21(myposition);
        
        
        {
        
            result_peg_21 = rule_typename_rpc(stream, result_peg_21.getPosition());
                if (result_peg_21.error()){
                    goto out_peg_23;
                }
            
            
            
            int save_peg_24 = result_peg_21.getPosition();
                {
                
                    int save_peg_26 = result_peg_21.getPosition();
                        
                        result_peg_21 = rule_tok_space(stream, result_peg_21.getPosition());
                        if (result_peg_21.error()){
                            
                            result_peg_21 = Result(save_peg_26);
                            result_peg_21.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_21 = rule_val_attrs(stream, result_peg_21.getPosition());
                        if (result_peg_21.error()){
                            
                            result_peg_21 = Result(save_peg_24);
                            result_peg_21.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = result_peg_21;
        stream.update(result_peg_21.getPosition());
        
        
        return result_peg_21;
        out_peg_23:
        Result result_peg_27(myposition);
        
        
        {
        
            result_peg_27 = rule_typename_proj(stream, result_peg_27.getPosition());
                if (result_peg_27.error()){
                    goto out_peg_29;
                }
            
            
            
            int save_peg_30 = result_peg_27.getPosition();
                {
                
                    int save_peg_32 = result_peg_27.getPosition();
                        
                        result_peg_27 = rule_tok_space(stream, result_peg_27.getPosition());
                        if (result_peg_27.error()){
                            
                            result_peg_27 = Result(save_peg_32);
                            result_peg_27.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_27 = rule_val_attrs(stream, result_peg_27.getPosition());
                        if (result_peg_27.error()){
                            
                            result_peg_27 = Result(save_peg_30);
                            result_peg_27.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = result_peg_27;
        stream.update(result_peg_27.getPosition());
        
        
        return result_peg_27;
        out_peg_29:
    
        if (column_peg_2.chunk8 == 0){
            column_peg_2.chunk8 = new Chunk8();
        }
        column_peg_2.chunk8->chunk_typename_any_no_ptr = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_any(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk9 != 0 && column_peg_2.chunk9->chunk_typename_any.calculated()){
        return column_peg_2.chunk9->chunk_typename_any;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_any");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_typename_arith(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_6 = result_peg_3.getPosition();
                {
                
                    int save_peg_8 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_8);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_val_attrs(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_6);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
        Result result_peg_9(myposition);
        
        
        {
        
            result_peg_9 = rule_typename_array(stream, result_peg_9.getPosition());
                if (result_peg_9.error()){
                    goto out_peg_11;
                }
            
            
            
            int save_peg_12 = result_peg_9.getPosition();
                {
                
                    int save_peg_14 = result_peg_9.getPosition();
                        
                        result_peg_9 = rule_tok_space(stream, result_peg_9.getPosition());
                        if (result_peg_9.error()){
                            
                            result_peg_9 = Result(save_peg_14);
                            result_peg_9.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_9 = rule_val_attrs(stream, result_peg_9.getPosition());
                        if (result_peg_9.error()){
                            
                            result_peg_9 = Result(save_peg_12);
                            result_peg_9.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_9;
        stream.update(result_peg_9.getPosition());
        
        
        return result_peg_9;
        out_peg_11:
        Result result_peg_15(myposition);
        
        
        {
        
            result_peg_15 = rule_typename_string(stream, result_peg_15.getPosition());
                if (result_peg_15.error()){
                    goto out_peg_17;
                }
            
            
            
            int save_peg_18 = result_peg_15.getPosition();
                {
                
                    int save_peg_20 = result_peg_15.getPosition();
                        
                        result_peg_15 = rule_tok_space(stream, result_peg_15.getPosition());
                        if (result_peg_15.error()){
                            
                            result_peg_15 = Result(save_peg_20);
                            result_peg_15.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_15 = rule_val_attrs(stream, result_peg_15.getPosition());
                        if (result_peg_15.error()){
                            
                            result_peg_15 = Result(save_peg_18);
                            result_peg_15.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_15;
        stream.update(result_peg_15.getPosition());
        
        
        return result_peg_15;
        out_peg_17:
        Result result_peg_21(myposition);
        
        
        {
        
            result_peg_21 = rule_typename_rpc(stream, result_peg_21.getPosition());
                if (result_peg_21.error()){
                    goto out_peg_23;
                }
            
            
            
            int save_peg_24 = result_peg_21.getPosition();
                {
                
                    int save_peg_26 = result_peg_21.getPosition();
                        
                        result_peg_21 = rule_tok_space(stream, result_peg_21.getPosition());
                        if (result_peg_21.error()){
                            
                            result_peg_21 = Result(save_peg_26);
                            result_peg_21.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_21 = rule_val_attrs(stream, result_peg_21.getPosition());
                        if (result_peg_21.error()){
                            
                            result_peg_21 = Result(save_peg_24);
                            result_peg_21.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_21;
        stream.update(result_peg_21.getPosition());
        
        
        return result_peg_21;
        out_peg_23:
        Result result_peg_27(myposition);
        
        
        {
        
            result_peg_27 = rule_typename_proj(stream, result_peg_27.getPosition());
                if (result_peg_27.error()){
                    goto out_peg_29;
                }
            
            
            
            int save_peg_30 = result_peg_27.getPosition();
                {
                
                    int save_peg_32 = result_peg_27.getPosition();
                        
                        result_peg_27 = rule_tok_space(stream, result_peg_27.getPosition());
                        if (result_peg_27.error()){
                            
                            result_peg_27 = Result(save_peg_32);
                            result_peg_27.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_27 = rule_val_attrs(stream, result_peg_27.getPosition());
                        if (result_peg_27.error()){
                            
                            result_peg_27 = Result(save_peg_30);
                            result_peg_27.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_27;
        stream.update(result_peg_27.getPosition());
        
        
        return result_peg_27;
        out_peg_29:
        Result result_peg_33(myposition);
        
        
        {
        
            result_peg_33 = rule_typename_ptr(stream, result_peg_33.getPosition());
                if (result_peg_33.error()){
                    goto out_peg_35;
                }
            
            
            
            int save_peg_36 = result_peg_33.getPosition();
                {
                
                    int save_peg_38 = result_peg_33.getPosition();
                        
                        result_peg_33 = rule_tok_space(stream, result_peg_33.getPosition());
                        if (result_peg_33.error()){
                            
                            result_peg_33 = Result(save_peg_38);
                            result_peg_33.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_33 = rule_val_attrs(stream, result_peg_33.getPosition());
                        if (result_peg_33.error()){
                            
                            result_peg_33 = Result(save_peg_36);
                            result_peg_33.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = result_peg_33;
        stream.update(result_peg_33.getPosition());
        
        
        return result_peg_33;
        out_peg_35:
    
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_any = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_arith(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk9 != 0 && column_peg_2.chunk9->chunk_typename_arith.calculated()){
        return column_peg_2.chunk9->chunk_typename_arith;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_arith");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_8 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_8);
                                    goto out_peg_9;
                                }
                            }
                                
                        }
                        goto success_peg_7;
                        out_peg_9:
                        
                        result_peg_3 = Result(save_peg_5);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_7:
                        ;
                    
                    
                    
                    result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_12 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_12);
                            goto out_peg_13;
                        }
                    }
                        
                }
                goto success_peg_11;
                out_peg_13:
                goto out_peg_14;
                success_peg_11:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            {
                    int position_peg_18 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_18);
                            goto out_peg_19;
                        }
                    }
                        
                }
                goto success_peg_17;
                out_peg_19:
                goto out_peg_14;
                success_peg_17:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            {
                    int position_peg_22 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_22);
                            goto out_peg_23;
                        }
                    }
                        
                }
                goto success_peg_21;
                out_peg_23:
                goto out_peg_14;
                success_peg_21:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_14:
        Result result_peg_24(myposition);
        
        
        {
        
            int save_peg_26 = result_peg_24.getPosition();
                {
                
                    {
                            int position_peg_29 = result_peg_24.getPosition();
                            
                            result_peg_24.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_24.getPosition()))){
                                    result_peg_24.nextPosition();
                                } else {
                                    result_peg_24.setPosition(position_peg_29);
                                    goto out_peg_30;
                                }
                            }
                                
                        }
                        goto success_peg_28;
                        out_peg_30:
                        
                        result_peg_24 = Result(save_peg_26);
                        result_peg_24.setValue(Value((void*) 0));
                        
                        success_peg_28:
                        ;
                    
                    
                    
                    result_peg_24 = rule_tok_space(stream, result_peg_24.getPosition());
                        if (result_peg_24.error()){
                            
                            result_peg_24 = Result(save_peg_26);
                            result_peg_24.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_33 = result_peg_24.getPosition();
                    
                    result_peg_24.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_24.getPosition()))){
                            result_peg_24.nextPosition();
                        } else {
                            result_peg_24.setPosition(position_peg_33);
                            goto out_peg_34;
                        }
                    }
                        
                }
                goto success_peg_32;
                out_peg_34:
                goto out_peg_35;
                success_peg_32:
                ;
            
            
            
            result_peg_24 = rule_tok_space(stream, result_peg_24.getPosition());
                if (result_peg_24.error()){
                    goto out_peg_35;
                }
            
            
            
            {
                    int position_peg_38 = result_peg_24.getPosition();
                    
                    result_peg_24.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_24.getPosition()))){
                            result_peg_24.nextPosition();
                        } else {
                            result_peg_24.setPosition(position_peg_38);
                            goto out_peg_39;
                        }
                    }
                        
                }
                goto success_peg_37;
                out_peg_39:
                goto out_peg_35;
                success_peg_37:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_24;
        stream.update(result_peg_24.getPosition());
        
        
        return result_peg_24;
        out_peg_35:
        Result result_peg_40(myposition);
        
        
        {
        
            int save_peg_42 = result_peg_40.getPosition();
                {
                
                    {
                            int position_peg_45 = result_peg_40.getPosition();
                            
                            result_peg_40.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_40.getPosition()))){
                                    result_peg_40.nextPosition();
                                } else {
                                    result_peg_40.setPosition(position_peg_45);
                                    goto out_peg_46;
                                }
                            }
                                
                        }
                        goto success_peg_44;
                        out_peg_46:
                        
                        result_peg_40 = Result(save_peg_42);
                        result_peg_40.setValue(Value((void*) 0));
                        
                        success_peg_44:
                        ;
                    
                    
                    
                    result_peg_40 = rule_tok_space(stream, result_peg_40.getPosition());
                        if (result_peg_40.error()){
                            
                            result_peg_40 = Result(save_peg_42);
                            result_peg_40.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_49 = result_peg_40.getPosition();
                    
                    result_peg_40.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_40.getPosition()))){
                            result_peg_40.nextPosition();
                        } else {
                            result_peg_40.setPosition(position_peg_49);
                            goto out_peg_50;
                        }
                    }
                        
                }
                goto success_peg_48;
                out_peg_50:
                goto out_peg_51;
                success_peg_48:
                ;
            
            
            
            result_peg_40 = rule_tok_space(stream, result_peg_40.getPosition());
                if (result_peg_40.error()){
                    goto out_peg_51;
                }
            
            
            
            {
                    int position_peg_54 = result_peg_40.getPosition();
                    
                    result_peg_40.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_40.getPosition()))){
                            result_peg_40.nextPosition();
                        } else {
                            result_peg_40.setPosition(position_peg_54);
                            goto out_peg_55;
                        }
                    }
                        
                }
                goto success_peg_53;
                out_peg_55:
                goto out_peg_51;
                success_peg_53:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_40;
        stream.update(result_peg_40.getPosition());
        
        
        return result_peg_40;
        out_peg_51:
        Result result_peg_56(myposition);
        
        
        {
        
            int save_peg_58 = result_peg_56.getPosition();
                {
                
                    {
                            int position_peg_61 = result_peg_56.getPosition();
                            
                            result_peg_56.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_56.getPosition()))){
                                    result_peg_56.nextPosition();
                                } else {
                                    result_peg_56.setPosition(position_peg_61);
                                    goto out_peg_62;
                                }
                            }
                                
                        }
                        goto success_peg_60;
                        out_peg_62:
                        
                        result_peg_56 = Result(save_peg_58);
                        result_peg_56.setValue(Value((void*) 0));
                        
                        success_peg_60:
                        ;
                    
                    
                    
                    result_peg_56 = rule_tok_space(stream, result_peg_56.getPosition());
                        if (result_peg_56.error()){
                            
                            result_peg_56 = Result(save_peg_58);
                            result_peg_56.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_65 = result_peg_56.getPosition();
                    
                    result_peg_56.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_56.getPosition()))){
                            result_peg_56.nextPosition();
                        } else {
                            result_peg_56.setPosition(position_peg_65);
                            goto out_peg_66;
                        }
                    }
                        
                }
                goto success_peg_64;
                out_peg_66:
                goto out_peg_67;
                success_peg_64:
                ;
            
            
            
            result_peg_56 = rule_tok_space(stream, result_peg_56.getPosition());
                if (result_peg_56.error()){
                    goto out_peg_67;
                }
            
            
            
            {
                    int position_peg_70 = result_peg_56.getPosition();
                    
                    result_peg_56.setValue(Value((void*) "int"));
                    for (int i = 0; i < 3; i++){
                        if (compareChar("int"[i], stream.get(result_peg_56.getPosition()))){
                            result_peg_56.nextPosition();
                        } else {
                            result_peg_56.setPosition(position_peg_70);
                            goto out_peg_71;
                        }
                    }
                        
                }
                goto success_peg_69;
                out_peg_71:
                goto out_peg_67;
                success_peg_69:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_56;
        stream.update(result_peg_56.getPosition());
        
        
        return result_peg_56;
        out_peg_67:
        Result result_peg_72(myposition);
        
        
        {
        
            int save_peg_74 = result_peg_72.getPosition();
                {
                
                    {
                            int position_peg_77 = result_peg_72.getPosition();
                            
                            result_peg_72.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_72.getPosition()))){
                                    result_peg_72.nextPosition();
                                } else {
                                    result_peg_72.setPosition(position_peg_77);
                                    goto out_peg_78;
                                }
                            }
                                
                        }
                        goto success_peg_76;
                        out_peg_78:
                        
                        result_peg_72 = Result(save_peg_74);
                        result_peg_72.setValue(Value((void*) 0));
                        
                        success_peg_76:
                        ;
                    
                    
                    
                    result_peg_72 = rule_tok_space(stream, result_peg_72.getPosition());
                        if (result_peg_72.error()){
                            
                            result_peg_72 = Result(save_peg_74);
                            result_peg_72.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_81 = result_peg_72.getPosition();
                    
                    result_peg_72.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_72.getPosition()))){
                            result_peg_72.nextPosition();
                        } else {
                            result_peg_72.setPosition(position_peg_81);
                            goto out_peg_82;
                        }
                    }
                        
                }
                goto success_peg_80;
                out_peg_82:
                goto out_peg_83;
                success_peg_80:
                ;
            
            
            
            result_peg_72 = rule_tok_space(stream, result_peg_72.getPosition());
                if (result_peg_72.error()){
                    goto out_peg_83;
                }
            
            
            
            {
                    int position_peg_86 = result_peg_72.getPosition();
                    
                    result_peg_72.setValue(Value((void*) "short"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("short"[i], stream.get(result_peg_72.getPosition()))){
                            result_peg_72.nextPosition();
                        } else {
                            result_peg_72.setPosition(position_peg_86);
                            goto out_peg_87;
                        }
                    }
                        
                }
                goto success_peg_85;
                out_peg_87:
                goto out_peg_83;
                success_peg_85:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_72;
        stream.update(result_peg_72.getPosition());
        
        
        return result_peg_72;
        out_peg_83:
        Result result_peg_88(myposition);
        
        
        {
        
            int save_peg_90 = result_peg_88.getPosition();
                {
                
                    {
                            int position_peg_93 = result_peg_88.getPosition();
                            
                            result_peg_88.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_88.getPosition()))){
                                    result_peg_88.nextPosition();
                                } else {
                                    result_peg_88.setPosition(position_peg_93);
                                    goto out_peg_94;
                                }
                            }
                                
                        }
                        goto success_peg_92;
                        out_peg_94:
                        
                        result_peg_88 = Result(save_peg_90);
                        result_peg_88.setValue(Value((void*) 0));
                        
                        success_peg_92:
                        ;
                    
                    
                    
                    result_peg_88 = rule_tok_space(stream, result_peg_88.getPosition());
                        if (result_peg_88.error()){
                            
                            result_peg_88 = Result(save_peg_90);
                            result_peg_88.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_97 = result_peg_88.getPosition();
                    
                    result_peg_88.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_88.getPosition()))){
                            result_peg_88.nextPosition();
                        } else {
                            result_peg_88.setPosition(position_peg_97);
                            goto out_peg_98;
                        }
                    }
                        
                }
                goto success_peg_96;
                out_peg_98:
                goto out_peg_99;
                success_peg_96:
                ;
            
            
            
            result_peg_88 = rule_tok_space(stream, result_peg_88.getPosition());
                if (result_peg_88.error()){
                    goto out_peg_99;
                }
            
            
            
            {
                    int position_peg_102 = result_peg_88.getPosition();
                    
                    result_peg_88.setValue(Value((void*) "char"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("char"[i], stream.get(result_peg_88.getPosition()))){
                            result_peg_88.nextPosition();
                        } else {
                            result_peg_88.setPosition(position_peg_102);
                            goto out_peg_103;
                        }
                    }
                        
                }
                goto success_peg_101;
                out_peg_103:
                goto out_peg_99;
                success_peg_101:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_88;
        stream.update(result_peg_88.getPosition());
        
        
        return result_peg_88;
        out_peg_99:
        Result result_peg_104(myposition);
        
        
        {
        
            int save_peg_106 = result_peg_104.getPosition();
                {
                
                    {
                            int position_peg_109 = result_peg_104.getPosition();
                            
                            result_peg_104.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_104.getPosition()))){
                                    result_peg_104.nextPosition();
                                } else {
                                    result_peg_104.setPosition(position_peg_109);
                                    goto out_peg_110;
                                }
                            }
                                
                        }
                        goto success_peg_108;
                        out_peg_110:
                        
                        result_peg_104 = Result(save_peg_106);
                        result_peg_104.setValue(Value((void*) 0));
                        
                        success_peg_108:
                        ;
                    
                    
                    
                    result_peg_104 = rule_tok_space(stream, result_peg_104.getPosition());
                        if (result_peg_104.error()){
                            
                            result_peg_104 = Result(save_peg_106);
                            result_peg_104.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_113 = result_peg_104.getPosition();
                    
                    result_peg_104.setValue(Value((void*) "signed"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("signed"[i], stream.get(result_peg_104.getPosition()))){
                            result_peg_104.nextPosition();
                        } else {
                            result_peg_104.setPosition(position_peg_113);
                            goto out_peg_114;
                        }
                    }
                        
                }
                goto success_peg_112;
                out_peg_114:
                goto out_peg_115;
                success_peg_112:
                ;
            
            
            
            result_peg_104 = rule_tok_space(stream, result_peg_104.getPosition());
                if (result_peg_104.error()){
                    goto out_peg_115;
                }
            
            
            
            {
                    int position_peg_118 = result_peg_104.getPosition();
                    
                    result_peg_104.setValue(Value((void*) "char"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("char"[i], stream.get(result_peg_104.getPosition()))){
                            result_peg_104.nextPosition();
                        } else {
                            result_peg_104.setPosition(position_peg_118);
                            goto out_peg_119;
                        }
                    }
                        
                }
                goto success_peg_117;
                out_peg_119:
                goto out_peg_115;
                success_peg_117:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_104;
        stream.update(result_peg_104.getPosition());
        
        
        return result_peg_104;
        out_peg_115:
        Result result_peg_120(myposition);
        
        
        {
        
            int save_peg_122 = result_peg_120.getPosition();
                {
                
                    {
                            int position_peg_125 = result_peg_120.getPosition();
                            
                            result_peg_120.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_120.getPosition()))){
                                    result_peg_120.nextPosition();
                                } else {
                                    result_peg_120.setPosition(position_peg_125);
                                    goto out_peg_126;
                                }
                            }
                                
                        }
                        goto success_peg_124;
                        out_peg_126:
                        
                        result_peg_120 = Result(save_peg_122);
                        result_peg_120.setValue(Value((void*) 0));
                        
                        success_peg_124:
                        ;
                    
                    
                    
                    result_peg_120 = rule_tok_space(stream, result_peg_120.getPosition());
                        if (result_peg_120.error()){
                            
                            result_peg_120 = Result(save_peg_122);
                            result_peg_120.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_128 = result_peg_120.getPosition();
                    
                    result_peg_120.setValue(Value((void*) "short"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("short"[i], stream.get(result_peg_120.getPosition()))){
                            result_peg_120.nextPosition();
                        } else {
                            result_peg_120.setPosition(position_peg_128);
                            goto out_peg_129;
                        }
                    }
                        
                }
                goto success_peg_127;
                out_peg_129:
                goto out_peg_130;
                success_peg_127:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_120;
        stream.update(result_peg_120.getPosition());
        
        
        return result_peg_120;
        out_peg_130:
        Result result_peg_131(myposition);
        
        
        {
        
            int save_peg_133 = result_peg_131.getPosition();
                {
                
                    {
                            int position_peg_136 = result_peg_131.getPosition();
                            
                            result_peg_131.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_131.getPosition()))){
                                    result_peg_131.nextPosition();
                                } else {
                                    result_peg_131.setPosition(position_peg_136);
                                    goto out_peg_137;
                                }
                            }
                                
                        }
                        goto success_peg_135;
                        out_peg_137:
                        
                        result_peg_131 = Result(save_peg_133);
                        result_peg_131.setValue(Value((void*) 0));
                        
                        success_peg_135:
                        ;
                    
                    
                    
                    result_peg_131 = rule_tok_space(stream, result_peg_131.getPosition());
                        if (result_peg_131.error()){
                            
                            result_peg_131 = Result(save_peg_133);
                            result_peg_131.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_139 = result_peg_131.getPosition();
                    
                    result_peg_131.setValue(Value((void*) "int"));
                    for (int i = 0; i < 3; i++){
                        if (compareChar("int"[i], stream.get(result_peg_131.getPosition()))){
                            result_peg_131.nextPosition();
                        } else {
                            result_peg_131.setPosition(position_peg_139);
                            goto out_peg_140;
                        }
                    }
                        
                }
                goto success_peg_138;
                out_peg_140:
                goto out_peg_141;
                success_peg_138:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_131;
        stream.update(result_peg_131.getPosition());
        
        
        return result_peg_131;
        out_peg_141:
        Result result_peg_142(myposition);
        
        
        {
        
            int save_peg_144 = result_peg_142.getPosition();
                {
                
                    {
                            int position_peg_147 = result_peg_142.getPosition();
                            
                            result_peg_142.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_142.getPosition()))){
                                    result_peg_142.nextPosition();
                                } else {
                                    result_peg_142.setPosition(position_peg_147);
                                    goto out_peg_148;
                                }
                            }
                                
                        }
                        goto success_peg_146;
                        out_peg_148:
                        
                        result_peg_142 = Result(save_peg_144);
                        result_peg_142.setValue(Value((void*) 0));
                        
                        success_peg_146:
                        ;
                    
                    
                    
                    result_peg_142 = rule_tok_space(stream, result_peg_142.getPosition());
                        if (result_peg_142.error()){
                            
                            result_peg_142 = Result(save_peg_144);
                            result_peg_142.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_150 = result_peg_142.getPosition();
                    
                    result_peg_142.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_142.getPosition()))){
                            result_peg_142.nextPosition();
                        } else {
                            result_peg_142.setPosition(position_peg_150);
                            goto out_peg_151;
                        }
                    }
                        
                }
                goto success_peg_149;
                out_peg_151:
                goto out_peg_152;
                success_peg_149:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_142;
        stream.update(result_peg_142.getPosition());
        
        
        return result_peg_142;
        out_peg_152:
        Result result_peg_153(myposition);
        
        
        {
        
            int save_peg_155 = result_peg_153.getPosition();
                {
                
                    {
                            int position_peg_158 = result_peg_153.getPosition();
                            
                            result_peg_153.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_153.getPosition()))){
                                    result_peg_153.nextPosition();
                                } else {
                                    result_peg_153.setPosition(position_peg_158);
                                    goto out_peg_159;
                                }
                            }
                                
                        }
                        goto success_peg_157;
                        out_peg_159:
                        
                        result_peg_153 = Result(save_peg_155);
                        result_peg_153.setValue(Value((void*) 0));
                        
                        success_peg_157:
                        ;
                    
                    
                    
                    result_peg_153 = rule_tok_space(stream, result_peg_153.getPosition());
                        if (result_peg_153.error()){
                            
                            result_peg_153 = Result(save_peg_155);
                            result_peg_153.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_161 = result_peg_153.getPosition();
                    
                    result_peg_153.setValue(Value((void*) "char"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("char"[i], stream.get(result_peg_153.getPosition()))){
                            result_peg_153.nextPosition();
                        } else {
                            result_peg_153.setPosition(position_peg_161);
                            goto out_peg_162;
                        }
                    }
                        
                }
                goto success_peg_160;
                out_peg_162:
                goto out_peg_163;
                success_peg_160:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = result_peg_153;
        stream.update(result_peg_153.getPosition());
        
        
        return result_peg_153;
        out_peg_163:
    
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_arith = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_array(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk9 != 0 && column_peg_2.chunk9->chunk_typename_array.calculated()){
        return column_peg_2.chunk9->chunk_typename_array;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_array");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_8 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_8);
                                    goto out_peg_9;
                                }
                            }
                                
                        }
                        goto success_peg_7;
                        out_peg_9:
                        
                        result_peg_3 = Result(save_peg_5);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_7:
                        ;
                    
                    
                    
                    result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_12 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "array"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("array"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_12);
                            goto out_peg_13;
                        }
                    }
                        
                }
                goto success_peg_11;
                out_peg_13:
                goto out_peg_14;
                success_peg_11:
                ;
            
            
            
            int save_peg_16 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_16);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_19 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "<"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("<"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_19);
                            goto out_peg_20;
                        }
                    }
                        
                }
                goto success_peg_18;
                out_peg_20:
                goto out_peg_14;
                success_peg_18:
                ;
            
            
            
            int save_peg_22 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_22);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_typename_any(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            int save_peg_25 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_25);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_28 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ","));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(","[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_28);
                            goto out_peg_29;
                        }
                    }
                        
                }
                goto success_peg_27;
                out_peg_29:
                goto out_peg_14;
                success_peg_27:
                ;
            
            
            
            int save_peg_31 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_31);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3 = rule_array_size(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            int save_peg_34 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_34);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_36 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ">"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(">"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_36);
                            goto out_peg_37;
                        }
                    }
                        
                }
                goto success_peg_35;
                out_peg_37:
                goto out_peg_14;
                success_peg_35:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_array = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_14:
    
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_array = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_array_size(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk9 != 0 && column_peg_2.chunk9->chunk_array_size.calculated()){
        return column_peg_2.chunk9->chunk_array_size;
    }
    
    
    RuleTrace trace_peg_1(stream, "array_size");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.reset();
            do{
                Result result_peg_7(result_peg_3.getPosition());
                char letter_peg_8 = stream.get(result_peg_7.getPosition());
                if (letter_peg_8 != '\0' && strchr("0123456789", letter_peg_8) != NULL){
                    result_peg_7.nextPosition();
                    result_peg_7.setValue(Value((void*) (intptr_t) letter_peg_8));
                } else {
                    goto loop_peg_6;
                }
                result_peg_3.addResult(result_peg_7);
            } while (true);
            loop_peg_6:
            if (result_peg_3.matches() == 0){
                result_peg_3.setPosition(position_peg_5);
                goto out_peg_9;
            }
            
        }
        goto success_peg_4;
        out_peg_9:
        goto out_peg_10;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_array_size = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_10:
        Result result_peg_11(myposition);
        
        
        result_peg_11 = rule_field_ref(stream, result_peg_11.getPosition());
        if (result_peg_11.error()){
            goto out_peg_12;
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_array_size = result_peg_11;
        stream.update(result_peg_11.getPosition());
        
        
        return result_peg_11;
        out_peg_12:
        Result result_peg_13(myposition);
        
        
        {
            int position_peg_15 = result_peg_13.getPosition();
            
            result_peg_13.setValue(Value((void*) "null"));
            for (int i = 0; i < 4; i++){
                if (compareChar("null"[i], stream.get(result_peg_13.getPosition()))){
                    result_peg_13.nextPosition();
                } else {
                    result_peg_13.setPosition(position_peg_15);
                    goto out_peg_16;
                }
            }
                
        }
        goto success_peg_14;
        out_peg_16:
        goto out_peg_17;
        success_peg_14:
        ;
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_array_size = result_peg_13;
        stream.update(result_peg_13.getPosition());
        
        
        return result_peg_13;
        out_peg_17:
    
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_array_size = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_string(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk9 != 0 && column_peg_2.chunk9->chunk_typename_string.calculated()){
        return column_peg_2.chunk9->chunk_typename_string;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_string");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_8 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_8);
                                    goto out_peg_9;
                                }
                            }
                                
                        }
                        goto success_peg_7;
                        out_peg_9:
                        
                        result_peg_3 = Result(save_peg_5);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_7:
                        ;
                    
                    
                    
                    result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_11 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "string"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("string"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_11);
                            goto out_peg_12;
                        }
                    }
                        
                }
                goto success_peg_10;
                out_peg_12:
                goto out_peg_13;
                success_peg_10:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_string = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_13:
    
        if (column_peg_2.chunk9 == 0){
            column_peg_2.chunk9 = new Chunk9();
        }
        column_peg_2.chunk9->chunk_typename_string = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_rpc(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk10 != 0 && column_peg_2.chunk10->chunk_typename_rpc.calculated()){
        return column_peg_2.chunk10->chunk_typename_rpc;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_rpc");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_8 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_8);
                                    goto out_peg_9;
                                }
                            }
                                
                        }
                        goto success_peg_7;
                        out_peg_9:
                        
                        result_peg_3 = Result(save_peg_5);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_7:
                        ;
                    
                    
                    
                    result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_12 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "rpc"));
                    for (int i = 0; i < 3; i++){
                        if (compareChar("rpc"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_12);
                            goto out_peg_13;
                        }
                    }
                        
                }
                goto success_peg_11;
                out_peg_13:
                goto out_peg_14;
                success_peg_11:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
        }
        
        
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_typename_rpc = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_14:
    
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_typename_rpc = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_typename_proj(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk10 != 0 && column_peg_2.chunk10->chunk_typename_proj.calculated()){
        return column_peg_2.chunk10->chunk_typename_proj;
    }
    
    
    RuleTrace trace_peg_1(stream, "typename_proj");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            int save_peg_5 = result_peg_3.getPosition();
                {
                
                    {
                            int position_peg_8 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "const"));
                            for (int i = 0; i < 5; i++){
                                if (compareChar("const"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_8);
                                    goto out_peg_9;
                                }
                            }
                                
                        }
                        goto success_peg_7;
                        out_peg_9:
                        
                        result_peg_3 = Result(save_peg_5);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_7:
                        ;
                    
                    
                    
                    result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_5);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            {
                    int position_peg_12 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "projection"));
                    for (int i = 0; i < 10; i++){
                        if (compareChar("projection"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_12);
                            goto out_peg_13;
                        }
                    }
                        
                }
                goto success_peg_11;
                out_peg_13:
                goto out_peg_14;
                success_peg_11:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_14;
                }
            
            
        }
        
        
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_typename_proj = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_14:
    
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_typename_proj = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_rpc_def(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk10 != 0 && column_peg_2.chunk10->chunk_rpc_def.calculated()){
        return column_peg_2.chunk10->chunk_rpc_def;
    }
    
    
    RuleTrace trace_peg_1(stream, "rpc_def");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "rpc"));
                    for (int i = 0; i < 3; i++){
                        if (compareChar("rpc"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_6);
                            goto out_peg_7;
                        }
                    }
                        
                }
                goto success_peg_5;
                out_peg_7:
                goto out_peg_8;
                success_peg_5:
                ;
            
            
            
            result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_tok_ident(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            int save_peg_12 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_12);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_15 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "{"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("{"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_15);
                            goto out_peg_16;
                        }
                    }
                        
                }
                goto success_peg_14;
                out_peg_16:
                goto out_peg_8;
                success_peg_14:
                ;
            
            
            
            int save_peg_18 = result_peg_3.getPosition();
                {
                
                    int save_peg_20 = result_peg_3.getPosition();
                        
                        result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_20);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3 = rule_rpc_item_list(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_18);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                }
            
            
            
            int save_peg_22 = result_peg_3.getPosition();
                
                result_peg_3 = rule_tok_space(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_22);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            {
                    int position_peg_24 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "}"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("}"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_24);
                            goto out_peg_25;
                        }
                    }
                        
                }
                goto success_peg_23;
                out_peg_25:
                goto out_peg_8;
                success_peg_23:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_def = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_def = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_rpc_item_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk10 != 0 && column_peg_2.chunk10->chunk_rpc_item_list.calculated()){
        return column_peg_2.chunk10->chunk_rpc_item_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "rpc_item_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_rpc_item(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                    
                        int save_peg_9 = result_peg_7.getPosition();
                            
                            result_peg_7 = rule_tok_space(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                
                                result_peg_7 = Result(save_peg_9);
                                result_peg_7.setValue(Value((void*) 0));
                                
                            }
                        
                        
                        
                        result_peg_7 = rule_rpc_item(stream, result_peg_7.getPosition());
                            if (result_peg_7.error()){
                                goto loop_peg_6;
                            }
                        
                        
                    }
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_item_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_item_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_rpc_item(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk10 != 0 && column_peg_2.chunk10->chunk_rpc_item.calculated()){
        return column_peg_2.chunk10->chunk_rpc_item;
    }
    
    
    RuleTrace trace_peg_1(stream, "rpc_item");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_proj_def(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_item = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
    
        if (column_peg_2.chunk10 == 0){
            column_peg_2.chunk10 = new Chunk10();
        }
        column_peg_2.chunk10->chunk_rpc_item = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

static const void * doParse(Stream & stream, bool stats, const std::string & context){
    errorResult.setError();
    Result done = rule_file(stream, 0);
    if (done.error()){
        stream.reportError(context);
    }
    if (stats){
        stream.printStats();
    }
    return done.getValues().getValue();
}

const void * parse(const std::string & filename, bool stats = false){
    Stream stream(filename);
    return doParse(stream, stats, filename);
}

const void * parse(const char * in, bool stats = false){
    Stream stream(in);
    return doParse(stream, stats, "memory");
}

const void * parse(const char * in, int length, bool stats = false){
    Stream stream(in, length);
    return doParse(stream, stats, "memory");
}



} /* Parser */

    