


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
Result chunk_whitespace;
    Result chunk_ident_cont;
    Result chunk_identifier;
    Result chunk_signed_type;
    Result chunk_unsigned_type;
};

struct Chunk1{
Result chunk_fp_type;
    Result chunk_primitive_type;
    Result chunk_simple_type;
    Result chunk_ret_type;
    Result chunk_addressable_type;
};

struct Chunk2{
Result chunk_pointer_type;
    Result chunk_proj_type;
    Result chunk_container_side;
    Result chunk_alloc_attrib;
    Result chunk_dealloc_attrib;
};

struct Chunk3{
Result chunk_bind_attrib;
    Result chunk_attribute;
    Result chunk_attrib_list;
    Result chunk_attrib_list_tail;
    Result chunk_attributes;
};

struct Chunk4{
Result chunk_rpc_decl;
    Result chunk_arg_list;
    Result chunk_arg_list_tail;
    Result chunk_argument;
    Result chunk_proj_field;
};

struct Chunk5{
Result chunk_proj_header;
    Result chunk_proj_body;
    Result chunk_projection;
    Result chunk_item;
    Result chunk_file;
};

struct Column{
    Column():
    chunk0(0)
        ,chunk1(0)
        ,chunk2(0)
        ,chunk3(0)
        ,chunk4(0)
        ,chunk5(0){
    }

    Chunk0 * chunk0;
    Chunk1 * chunk1;
    Chunk2 * chunk2;
    Chunk3 * chunk3;
    Chunk4 * chunk4;
    Chunk5 * chunk5;

    int hitCount(){
        return 0;
    }

    int maxHits(){
        return 30;
    }

    ~Column(){
        delete chunk0;
        delete chunk1;
        delete chunk2;
        delete chunk3;
        delete chunk4;
        delete chunk5;
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

Result rule_whitespace(Stream &, const int);
Result rule_ident_cont(Stream &, const int);
Result rule_identifier(Stream &, const int);
Result rule_signed_type(Stream &, const int);
Result rule_unsigned_type(Stream &, const int);
Result rule_fp_type(Stream &, const int);
Result rule_primitive_type(Stream &, const int);
Result rule_simple_type(Stream &, const int);
Result rule_ret_type(Stream &, const int);
Result rule_addressable_type(Stream &, const int);
Result rule_pointer_type(Stream &, const int);
Result rule_proj_type(Stream &, const int);
Result rule_container_side(Stream &, const int);
Result rule_alloc_attrib(Stream &, const int);
Result rule_dealloc_attrib(Stream &, const int);
Result rule_bind_attrib(Stream &, const int);
Result rule_attribute(Stream &, const int);
Result rule_attrib_list(Stream &, const int);
Result rule_attrib_list_tail(Stream &, const int);
Result rule_attributes(Stream &, const int);
Result rule_rpc_decl(Stream &, const int);
Result rule_arg_list(Stream &, const int);
Result rule_arg_list_tail(Stream &, const int);
Result rule_argument(Stream &, const int);
Result rule_proj_field(Stream &, const int);
Result rule_proj_header(Stream &, const int);
Result rule_proj_body(Stream &, const int);
Result rule_projection(Stream &, const int);
Result rule_item(Stream &, const int);
Result rule_file(Stream &, const int);




Result rule_whitespace(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_whitespace.calculated()){
        return column_peg_2.chunk0->chunk_whitespace;
    }
    
    
    RuleTrace trace_peg_1(stream, "whitespace");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3.reset();
        do{
            Result result_peg_5(result_peg_3.getPosition());
            {
                int position_peg_7 = result_peg_5.getPosition();
                
                char letter_peg_8 = stream.get(result_peg_5.getPosition());
                if (letter_peg_8 != '\0' && strchr(" \n\t", letter_peg_8) != NULL){
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
        column_peg_2.chunk0->chunk_whitespace = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_10:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_whitespace = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_ident_cont(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_ident_cont.calculated()){
        return column_peg_2.chunk0->chunk_ident_cont;
    }
    
    
    RuleTrace trace_peg_1(stream, "ident_cont");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            char letter_peg_6 = stream.get(result_peg_3.getPosition());
            if (letter_peg_6 != '\0' && strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", letter_peg_6) != NULL){
                result_peg_3.nextPosition();
                result_peg_3.setValue(Value((void*) (intptr_t) letter_peg_6));
            } else {
                result_peg_3.setPosition(position_peg_5);
                goto out_peg_7;
            }
            
        }
        goto success_peg_4;
        out_peg_7:
        goto out_peg_8;
        success_peg_4:
        ;
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_ident_cont = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
        Result result_peg_9(myposition);
        
        
        char letter_peg_10 = stream.get(result_peg_9.getPosition());
        if (letter_peg_10 != '\0' && strchr("0123456789", letter_peg_10) != NULL){
            result_peg_9.nextPosition();
            result_peg_9.setValue(Value((void*) (intptr_t) letter_peg_10));
        } else {
            goto out_peg_11;
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_ident_cont = result_peg_9;
        stream.update(result_peg_9.getPosition());
        
        
        return result_peg_9;
        out_peg_11:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_ident_cont = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_identifier(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_identifier.calculated()){
        return column_peg_2.chunk0->chunk_identifier;
    }
    
    
    RuleTrace trace_peg_1(stream, "identifier");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    char letter_peg_7 = stream.get(result_peg_3.getPosition());
                    if (letter_peg_7 != '\0' && strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", letter_peg_7) != NULL){
                        result_peg_3.nextPosition();
                        result_peg_3.setValue(Value((void*) (intptr_t) letter_peg_7));
                    } else {
                        result_peg_3.setPosition(position_peg_6);
                        goto out_peg_8;
                    }
                    
                }
                goto success_peg_5;
                out_peg_8:
                goto out_peg_9;
                success_peg_5:
                ;
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    result_peg_11 = rule_ident_cont(stream, result_peg_11.getPosition());
                    if (result_peg_11.error()){
                        goto loop_peg_10;
                    }
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_identifier = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_9:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_identifier = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_signed_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_signed_type.calculated()){
        return column_peg_2.chunk0->chunk_signed_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "signed_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "int"));
            for (int i = 0; i < 3; i++){
                if (compareChar("int"[i], stream.get(result_peg_3.getPosition()))){
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
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "short"));
            for (int i = 0; i < 5; i++){
                if (compareChar("short"[i], stream.get(result_peg_8.getPosition()))){
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
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
        Result result_peg_13(myposition);
        
        
        {
            int position_peg_15 = result_peg_13.getPosition();
            
            result_peg_13.setValue(Value((void*) "char"));
            for (int i = 0; i < 4; i++){
                if (compareChar("char"[i], stream.get(result_peg_13.getPosition()))){
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
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = result_peg_13;
        stream.update(result_peg_13.getPosition());
        
        
        return result_peg_13;
        out_peg_17:
        Result result_peg_18(myposition);
        
        
        {
        
            {
                    int position_peg_21 = result_peg_18.getPosition();
                    
                    result_peg_18.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_18.getPosition()))){
                            result_peg_18.nextPosition();
                        } else {
                            result_peg_18.setPosition(position_peg_21);
                            goto out_peg_22;
                        }
                    }
                        
                }
                goto success_peg_20;
                out_peg_22:
                goto out_peg_23;
                success_peg_20:
                ;
            
            
            
            result_peg_18 = rule_whitespace(stream, result_peg_18.getPosition());
                if (result_peg_18.error()){
                    goto out_peg_23;
                }
            
            
            
            {
                    int position_peg_26 = result_peg_18.getPosition();
                    
                    result_peg_18.setValue(Value((void*) "long"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("long"[i], stream.get(result_peg_18.getPosition()))){
                            result_peg_18.nextPosition();
                        } else {
                            result_peg_18.setPosition(position_peg_26);
                            goto out_peg_27;
                        }
                    }
                        
                }
                goto success_peg_25;
                out_peg_27:
                goto out_peg_23;
                success_peg_25:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = result_peg_18;
        stream.update(result_peg_18.getPosition());
        
        
        return result_peg_18;
        out_peg_23:
        Result result_peg_28(myposition);
        
        
        {
            int position_peg_30 = result_peg_28.getPosition();
            
            result_peg_28.setValue(Value((void*) "long"));
            for (int i = 0; i < 4; i++){
                if (compareChar("long"[i], stream.get(result_peg_28.getPosition()))){
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
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = result_peg_28;
        stream.update(result_peg_28.getPosition());
        
        
        return result_peg_28;
        out_peg_32:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_signed_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_unsigned_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk0 != 0 && column_peg_2.chunk0->chunk_unsigned_type.calculated()){
        return column_peg_2.chunk0->chunk_unsigned_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "unsigned_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "unsigned"));
                    for (int i = 0; i < 8; i++){
                        if (compareChar("unsigned"[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_signed_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_unsigned_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk0 == 0){
            column_peg_2.chunk0 = new Chunk0();
        }
        column_peg_2.chunk0->chunk_unsigned_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_fp_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_fp_type.calculated()){
        return column_peg_2.chunk1->chunk_fp_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "fp_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
            int position_peg_5 = result_peg_3.getPosition();
            
            result_peg_3.setValue(Value((void*) "float"));
            for (int i = 0; i < 5; i++){
                if (compareChar("float"[i], stream.get(result_peg_3.getPosition()))){
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
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_fp_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_7:
        Result result_peg_8(myposition);
        
        
        {
            int position_peg_10 = result_peg_8.getPosition();
            
            result_peg_8.setValue(Value((void*) "double"));
            for (int i = 0; i < 6; i++){
                if (compareChar("double"[i], stream.get(result_peg_8.getPosition()))){
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
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_fp_type = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_fp_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_primitive_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_primitive_type.calculated()){
        return column_peg_2.chunk1->chunk_primitive_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "primitive_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_signed_type(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_primitive_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_unsigned_type(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_primitive_type = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
        Result result_peg_7(myposition);
        
        
        result_peg_7 = rule_fp_type(stream, result_peg_7.getPosition());
        if (result_peg_7.error()){
            goto out_peg_8;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_primitive_type = result_peg_7;
        stream.update(result_peg_7.getPosition());
        
        
        return result_peg_7;
        out_peg_8:
        Result result_peg_9(myposition);
        
        
        {
            int position_peg_11 = result_peg_9.getPosition();
            
            result_peg_9.setValue(Value((void*) "bool"));
            for (int i = 0; i < 4; i++){
                if (compareChar("bool"[i], stream.get(result_peg_9.getPosition()))){
                    result_peg_9.nextPosition();
                } else {
                    result_peg_9.setPosition(position_peg_11);
                    goto out_peg_12;
                }
            }
                
        }
        goto success_peg_10;
        out_peg_12:
        goto out_peg_13;
        success_peg_10:
        ;
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_primitive_type = result_peg_9;
        stream.update(result_peg_9.getPosition());
        
        
        return result_peg_9;
        out_peg_13:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_primitive_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_simple_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_simple_type.calculated()){
        return column_peg_2.chunk1->chunk_simple_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "simple_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_pointer_type(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_simple_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_primitive_type(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_simple_type = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
        Result result_peg_7(myposition);
        
        
        result_peg_7 = rule_proj_type(stream, result_peg_7.getPosition());
        if (result_peg_7.error()){
            goto out_peg_8;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_simple_type = result_peg_7;
        stream.update(result_peg_7.getPosition());
        
        
        return result_peg_7;
        out_peg_8:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_simple_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_ret_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_ret_type.calculated()){
        return column_peg_2.chunk1->chunk_ret_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "ret_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_simple_type(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_ret_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        {
            int position_peg_7 = result_peg_5.getPosition();
            
            result_peg_5.setValue(Value((void*) "void"));
            for (int i = 0; i < 4; i++){
                if (compareChar("void"[i], stream.get(result_peg_5.getPosition()))){
                    result_peg_5.nextPosition();
                } else {
                    result_peg_5.setPosition(position_peg_7);
                    goto out_peg_8;
                }
            }
                
        }
        goto success_peg_6;
        out_peg_8:
        goto out_peg_9;
        success_peg_6:
        ;
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_ret_type = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_9:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_ret_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_addressable_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk1 != 0 && column_peg_2.chunk1->chunk_addressable_type.calculated()){
        return column_peg_2.chunk1->chunk_addressable_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "addressable_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_primitive_type(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_addressable_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_proj_type(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_addressable_type = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
        Result result_peg_7(myposition);
        
        
        {
            int position_peg_9 = result_peg_7.getPosition();
            
            result_peg_7.setValue(Value((void*) "void"));
            for (int i = 0; i < 4; i++){
                if (compareChar("void"[i], stream.get(result_peg_7.getPosition()))){
                    result_peg_7.nextPosition();
                } else {
                    result_peg_7.setPosition(position_peg_9);
                    goto out_peg_10;
                }
            }
                
        }
        goto success_peg_8;
        out_peg_10:
        goto out_peg_11;
        success_peg_8:
        ;
        
        
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_addressable_type = result_peg_7;
        stream.update(result_peg_7.getPosition());
        
        
        return result_peg_7;
        out_peg_11:
    
        if (column_peg_2.chunk1 == 0){
            column_peg_2.chunk1 = new Chunk1();
        }
        column_peg_2.chunk1->chunk_addressable_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_pointer_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_pointer_type.calculated()){
        return column_peg_2.chunk2->chunk_pointer_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "pointer_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_addressable_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_7(result_peg_3.getPosition());
                    {
                        int position_peg_9 = result_peg_7.getPosition();
                        
                        result_peg_7.setValue(Value((void*) "*"));
                        for (int i = 0; i < 1; i++){
                            if (compareChar("*"[i], stream.get(result_peg_7.getPosition()))){
                                result_peg_7.nextPosition();
                            } else {
                                result_peg_7.setPosition(position_peg_9);
                                goto out_peg_10;
                            }
                        }
                            
                    }
                    goto success_peg_8;
                    out_peg_10:
                    goto loop_peg_6;
                    success_peg_8:
                    ;
                    result_peg_3.addResult(result_peg_7);
                } while (true);
                loop_peg_6:
                if (result_peg_3.matches() == 0){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_pointer_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_pointer_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_type(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_proj_type.calculated()){
        return column_peg_2.chunk2->chunk_proj_type;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_type");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "proj"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("proj"[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_proj_type = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_proj_type = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_container_side(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_container_side.calculated()){
        return column_peg_2.chunk2->chunk_container_side;
    }
    
    
    RuleTrace trace_peg_1(stream, "container_side");
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
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_container_side = result_peg_3;
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
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_container_side = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_container_side = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_alloc_attrib(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_alloc_attrib.calculated()){
        return column_peg_2.chunk2->chunk_alloc_attrib;
    }
    
    
    RuleTrace trace_peg_1(stream, "alloc_attrib");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "alloc"));
                    for (int i = 0; i < 5; i++){
                        if (compareChar("alloc"[i], stream.get(result_peg_3.getPosition()))){
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
                
                    result_peg_3.reset();
                        do{
                            Result result_peg_12(result_peg_3.getPosition());
                            {
                                int position_peg_14 = result_peg_12.getPosition();
                                
                                char letter_peg_15 = stream.get(result_peg_12.getPosition());
                                if (letter_peg_15 != '\0' && strchr(" \n\t", letter_peg_15) != NULL){
                                    result_peg_12.nextPosition();
                                    result_peg_12.setValue(Value((void*) (intptr_t) letter_peg_15));
                                } else {
                                    result_peg_12.setPosition(position_peg_14);
                                    goto out_peg_16;
                                }
                                
                            }
                            goto success_peg_13;
                            out_peg_16:
                            goto loop_peg_11;
                            success_peg_13:
                            ;
                            result_peg_3.addResult(result_peg_12);
                        } while (true);
                        loop_peg_11:
                        ;
                    
                    
                    
                    {
                            int position_peg_19 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_19);
                                    goto out_peg_20;
                                }
                            }
                                
                        }
                        goto success_peg_18;
                        out_peg_20:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_18:
                        ;
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_23(result_peg_3.getPosition());
                            {
                                int position_peg_25 = result_peg_23.getPosition();
                                
                                char letter_peg_26 = stream.get(result_peg_23.getPosition());
                                if (letter_peg_26 != '\0' && strchr(" \n\t", letter_peg_26) != NULL){
                                    result_peg_23.nextPosition();
                                    result_peg_23.setValue(Value((void*) (intptr_t) letter_peg_26));
                                } else {
                                    result_peg_23.setPosition(position_peg_25);
                                    goto out_peg_27;
                                }
                                
                            }
                            goto success_peg_24;
                            out_peg_27:
                            goto loop_peg_22;
                            success_peg_24:
                            ;
                            result_peg_3.addResult(result_peg_23);
                        } while (true);
                        loop_peg_22:
                        ;
                    
                    
                    
                    result_peg_3 = rule_container_side(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_9);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_31(result_peg_3.getPosition());
                            {
                                int position_peg_33 = result_peg_31.getPosition();
                                
                                char letter_peg_34 = stream.get(result_peg_31.getPosition());
                                if (letter_peg_34 != '\0' && strchr(" \n\t", letter_peg_34) != NULL){
                                    result_peg_31.nextPosition();
                                    result_peg_31.setValue(Value((void*) (intptr_t) letter_peg_34));
                                } else {
                                    result_peg_31.setPosition(position_peg_33);
                                    goto out_peg_35;
                                }
                                
                            }
                            goto success_peg_32;
                            out_peg_35:
                            goto loop_peg_30;
                            success_peg_32:
                            ;
                            result_peg_3.addResult(result_peg_31);
                        } while (true);
                        loop_peg_30:
                        ;
                    
                    
                    
                    {
                            int position_peg_37 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_37);
                                    goto out_peg_38;
                                }
                            }
                                
                        }
                        goto success_peg_36;
                        out_peg_38:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_36:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_alloc_attrib = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_alloc_attrib = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_dealloc_attrib(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk2 != 0 && column_peg_2.chunk2->chunk_dealloc_attrib.calculated()){
        return column_peg_2.chunk2->chunk_dealloc_attrib;
    }
    
    
    RuleTrace trace_peg_1(stream, "dealloc_attrib");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "dealloc"));
                    for (int i = 0; i < 7; i++){
                        if (compareChar("dealloc"[i], stream.get(result_peg_3.getPosition()))){
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
                
                    result_peg_3.reset();
                        do{
                            Result result_peg_12(result_peg_3.getPosition());
                            {
                                int position_peg_14 = result_peg_12.getPosition();
                                
                                char letter_peg_15 = stream.get(result_peg_12.getPosition());
                                if (letter_peg_15 != '\0' && strchr(" \n\t", letter_peg_15) != NULL){
                                    result_peg_12.nextPosition();
                                    result_peg_12.setValue(Value((void*) (intptr_t) letter_peg_15));
                                } else {
                                    result_peg_12.setPosition(position_peg_14);
                                    goto out_peg_16;
                                }
                                
                            }
                            goto success_peg_13;
                            out_peg_16:
                            goto loop_peg_11;
                            success_peg_13:
                            ;
                            result_peg_3.addResult(result_peg_12);
                        } while (true);
                        loop_peg_11:
                        ;
                    
                    
                    
                    {
                            int position_peg_19 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_19);
                                    goto out_peg_20;
                                }
                            }
                                
                        }
                        goto success_peg_18;
                        out_peg_20:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_18:
                        ;
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_23(result_peg_3.getPosition());
                            {
                                int position_peg_25 = result_peg_23.getPosition();
                                
                                char letter_peg_26 = stream.get(result_peg_23.getPosition());
                                if (letter_peg_26 != '\0' && strchr(" \n\t", letter_peg_26) != NULL){
                                    result_peg_23.nextPosition();
                                    result_peg_23.setValue(Value((void*) (intptr_t) letter_peg_26));
                                } else {
                                    result_peg_23.setPosition(position_peg_25);
                                    goto out_peg_27;
                                }
                                
                            }
                            goto success_peg_24;
                            out_peg_27:
                            goto loop_peg_22;
                            success_peg_24:
                            ;
                            result_peg_3.addResult(result_peg_23);
                        } while (true);
                        loop_peg_22:
                        ;
                    
                    
                    
                    result_peg_3 = rule_container_side(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_9);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_31(result_peg_3.getPosition());
                            {
                                int position_peg_33 = result_peg_31.getPosition();
                                
                                char letter_peg_34 = stream.get(result_peg_31.getPosition());
                                if (letter_peg_34 != '\0' && strchr(" \n\t", letter_peg_34) != NULL){
                                    result_peg_31.nextPosition();
                                    result_peg_31.setValue(Value((void*) (intptr_t) letter_peg_34));
                                } else {
                                    result_peg_31.setPosition(position_peg_33);
                                    goto out_peg_35;
                                }
                                
                            }
                            goto success_peg_32;
                            out_peg_35:
                            goto loop_peg_30;
                            success_peg_32:
                            ;
                            result_peg_3.addResult(result_peg_31);
                        } while (true);
                        loop_peg_30:
                        ;
                    
                    
                    
                    {
                            int position_peg_37 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_37);
                                    goto out_peg_38;
                                }
                            }
                                
                        }
                        goto success_peg_36;
                        out_peg_38:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_36:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_dealloc_attrib = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk2 == 0){
            column_peg_2.chunk2 = new Chunk2();
        }
        column_peg_2.chunk2->chunk_dealloc_attrib = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_bind_attrib(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_bind_attrib.calculated()){
        return column_peg_2.chunk3->chunk_bind_attrib;
    }
    
    
    RuleTrace trace_peg_1(stream, "bind_attrib");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "bind"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("bind"[i], stream.get(result_peg_3.getPosition()))){
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
                
                    result_peg_3.reset();
                        do{
                            Result result_peg_12(result_peg_3.getPosition());
                            {
                                int position_peg_14 = result_peg_12.getPosition();
                                
                                char letter_peg_15 = stream.get(result_peg_12.getPosition());
                                if (letter_peg_15 != '\0' && strchr(" \n\t", letter_peg_15) != NULL){
                                    result_peg_12.nextPosition();
                                    result_peg_12.setValue(Value((void*) (intptr_t) letter_peg_15));
                                } else {
                                    result_peg_12.setPosition(position_peg_14);
                                    goto out_peg_16;
                                }
                                
                            }
                            goto success_peg_13;
                            out_peg_16:
                            goto loop_peg_11;
                            success_peg_13:
                            ;
                            result_peg_3.addResult(result_peg_12);
                        } while (true);
                        loop_peg_11:
                        ;
                    
                    
                    
                    {
                            int position_peg_19 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) "("));
                            for (int i = 0; i < 1; i++){
                                if (compareChar("("[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_19);
                                    goto out_peg_20;
                                }
                            }
                                
                        }
                        goto success_peg_18;
                        out_peg_20:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_18:
                        ;
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_23(result_peg_3.getPosition());
                            {
                                int position_peg_25 = result_peg_23.getPosition();
                                
                                char letter_peg_26 = stream.get(result_peg_23.getPosition());
                                if (letter_peg_26 != '\0' && strchr(" \n\t", letter_peg_26) != NULL){
                                    result_peg_23.nextPosition();
                                    result_peg_23.setValue(Value((void*) (intptr_t) letter_peg_26));
                                } else {
                                    result_peg_23.setPosition(position_peg_25);
                                    goto out_peg_27;
                                }
                                
                            }
                            goto success_peg_24;
                            out_peg_27:
                            goto loop_peg_22;
                            success_peg_24:
                            ;
                            result_peg_3.addResult(result_peg_23);
                        } while (true);
                        loop_peg_22:
                        ;
                    
                    
                    
                    result_peg_3 = rule_container_side(stream, result_peg_3.getPosition());
                        if (result_peg_3.error()){
                            
                            result_peg_3 = Result(save_peg_9);
                            result_peg_3.setValue(Value((void*) 0));
                            
                        }
                    
                    
                    
                    result_peg_3.reset();
                        do{
                            Result result_peg_31(result_peg_3.getPosition());
                            {
                                int position_peg_33 = result_peg_31.getPosition();
                                
                                char letter_peg_34 = stream.get(result_peg_31.getPosition());
                                if (letter_peg_34 != '\0' && strchr(" \n\t", letter_peg_34) != NULL){
                                    result_peg_31.nextPosition();
                                    result_peg_31.setValue(Value((void*) (intptr_t) letter_peg_34));
                                } else {
                                    result_peg_31.setPosition(position_peg_33);
                                    goto out_peg_35;
                                }
                                
                            }
                            goto success_peg_32;
                            out_peg_35:
                            goto loop_peg_30;
                            success_peg_32:
                            ;
                            result_peg_3.addResult(result_peg_31);
                        } while (true);
                        loop_peg_30:
                        ;
                    
                    
                    
                    {
                            int position_peg_37 = result_peg_3.getPosition();
                            
                            result_peg_3.setValue(Value((void*) ")"));
                            for (int i = 0; i < 1; i++){
                                if (compareChar(")"[i], stream.get(result_peg_3.getPosition()))){
                                    result_peg_3.nextPosition();
                                } else {
                                    result_peg_3.setPosition(position_peg_37);
                                    goto out_peg_38;
                                }
                            }
                                
                        }
                        goto success_peg_36;
                        out_peg_38:
                        
                        result_peg_3 = Result(save_peg_9);
                        result_peg_3.setValue(Value((void*) 0));
                        
                        success_peg_36:
                        ;
                    
                    
                }
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_bind_attrib = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_bind_attrib = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_attribute(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_attribute.calculated()){
        return column_peg_2.chunk3->chunk_attribute;
    }
    
    
    RuleTrace trace_peg_1(stream, "attribute");
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
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = result_peg_3;
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
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = result_peg_8;
        stream.update(result_peg_8.getPosition());
        
        
        return result_peg_8;
        out_peg_12:
        Result result_peg_13(myposition);
        
        
        result_peg_13 = rule_alloc_attrib(stream, result_peg_13.getPosition());
        if (result_peg_13.error()){
            goto out_peg_14;
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = result_peg_13;
        stream.update(result_peg_13.getPosition());
        
        
        return result_peg_13;
        out_peg_14:
        Result result_peg_15(myposition);
        
        
        result_peg_15 = rule_dealloc_attrib(stream, result_peg_15.getPosition());
        if (result_peg_15.error()){
            goto out_peg_16;
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = result_peg_15;
        stream.update(result_peg_15.getPosition());
        
        
        return result_peg_15;
        out_peg_16:
        Result result_peg_17(myposition);
        
        
        result_peg_17 = rule_bind_attrib(stream, result_peg_17.getPosition());
        if (result_peg_17.error()){
            goto out_peg_18;
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = result_peg_17;
        stream.update(result_peg_17.getPosition());
        
        
        return result_peg_17;
        out_peg_18:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attribute = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_attrib_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_attrib_list.calculated()){
        return column_peg_2.chunk3->chunk_attrib_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "attrib_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_attribute(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_8(result_peg_3.getPosition());
                    {
                        int position_peg_10 = result_peg_8.getPosition();
                        
                        char letter_peg_11 = stream.get(result_peg_8.getPosition());
                        if (letter_peg_11 != '\0' && strchr(" \n\t", letter_peg_11) != NULL){
                            result_peg_8.nextPosition();
                            result_peg_8.setValue(Value((void*) (intptr_t) letter_peg_11));
                        } else {
                            result_peg_8.setPosition(position_peg_10);
                            goto out_peg_12;
                        }
                        
                    }
                    goto success_peg_9;
                    out_peg_12:
                    goto loop_peg_7;
                    success_peg_9:
                    ;
                    result_peg_3.addResult(result_peg_8);
                } while (true);
                loop_peg_7:
                ;
            
            
            
            result_peg_3 = rule_attrib_list_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attrib_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attrib_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_attrib_list_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_attrib_list_tail.calculated()){
        return column_peg_2.chunk3->chunk_attrib_list_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "attrib_list_tail");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ","));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(","[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3 = rule_attrib_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attrib_list_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
        Result result_peg_16(myposition);
        
        
        
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attrib_list_tail = result_peg_16;
        stream.update(result_peg_16.getPosition());
        
        
        return result_peg_16;
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attrib_list_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_attributes(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk3 != 0 && column_peg_2.chunk3->chunk_attributes.calculated()){
        return column_peg_2.chunk3->chunk_attributes;
    }
    
    
    RuleTrace trace_peg_1(stream, "attributes");
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3 = rule_attrib_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_19(result_peg_3.getPosition());
                    {
                        int position_peg_21 = result_peg_19.getPosition();
                        
                        char letter_peg_22 = stream.get(result_peg_19.getPosition());
                        if (letter_peg_22 != '\0' && strchr(" \n\t", letter_peg_22) != NULL){
                            result_peg_19.nextPosition();
                            result_peg_19.setValue(Value((void*) (intptr_t) letter_peg_22));
                        } else {
                            result_peg_19.setPosition(position_peg_21);
                            goto out_peg_23;
                        }
                        
                    }
                    goto success_peg_20;
                    out_peg_23:
                    goto loop_peg_18;
                    success_peg_20:
                    ;
                    result_peg_3.addResult(result_peg_19);
                } while (true);
                loop_peg_18:
                ;
            
            
            
            {
                    int position_peg_25 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "]"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("]"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_25);
                            goto out_peg_26;
                        }
                    }
                        
                }
                goto success_peg_24;
                out_peg_26:
                goto out_peg_8;
                success_peg_24:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attributes = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk3 == 0){
            column_peg_2.chunk3 = new Chunk3();
        }
        column_peg_2.chunk3->chunk_attributes = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_rpc_decl(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_rpc_decl.calculated()){
        return column_peg_2.chunk4->chunk_rpc_decl;
    }
    
    
    RuleTrace trace_peg_1(stream, "rpc_decl");
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
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_ret_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_15(result_peg_3.getPosition());
                    {
                        int position_peg_17 = result_peg_15.getPosition();
                        
                        char letter_peg_18 = stream.get(result_peg_15.getPosition());
                        if (letter_peg_18 != '\0' && strchr(" \n\t", letter_peg_18) != NULL){
                            result_peg_15.nextPosition();
                            result_peg_15.setValue(Value((void*) (intptr_t) letter_peg_18));
                        } else {
                            result_peg_15.setPosition(position_peg_17);
                            goto out_peg_19;
                        }
                        
                    }
                    goto success_peg_16;
                    out_peg_19:
                    goto loop_peg_14;
                    success_peg_16:
                    ;
                    result_peg_3.addResult(result_peg_15);
                } while (true);
                loop_peg_14:
                ;
            
            
            
            {
                    int position_peg_22 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "("));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("("[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_26(result_peg_3.getPosition());
                    {
                        int position_peg_28 = result_peg_26.getPosition();
                        
                        char letter_peg_29 = stream.get(result_peg_26.getPosition());
                        if (letter_peg_29 != '\0' && strchr(" \n\t", letter_peg_29) != NULL){
                            result_peg_26.nextPosition();
                            result_peg_26.setValue(Value((void*) (intptr_t) letter_peg_29));
                        } else {
                            result_peg_26.setPosition(position_peg_28);
                            goto out_peg_30;
                        }
                        
                    }
                    goto success_peg_27;
                    out_peg_30:
                    goto loop_peg_25;
                    success_peg_27:
                    ;
                    result_peg_3.addResult(result_peg_26);
                } while (true);
                loop_peg_25:
                ;
            
            
            
            result_peg_3 = rule_arg_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_34(result_peg_3.getPosition());
                    {
                        int position_peg_36 = result_peg_34.getPosition();
                        
                        char letter_peg_37 = stream.get(result_peg_34.getPosition());
                        if (letter_peg_37 != '\0' && strchr(" \n\t", letter_peg_37) != NULL){
                            result_peg_34.nextPosition();
                            result_peg_34.setValue(Value((void*) (intptr_t) letter_peg_37));
                        } else {
                            result_peg_34.setPosition(position_peg_36);
                            goto out_peg_38;
                        }
                        
                    }
                    goto success_peg_35;
                    out_peg_38:
                    goto loop_peg_33;
                    success_peg_35:
                    ;
                    result_peg_3.addResult(result_peg_34);
                } while (true);
                loop_peg_33:
                ;
            
            
            
            {
                    int position_peg_41 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ")"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(")"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_41);
                            goto out_peg_42;
                        }
                    }
                        
                }
                goto success_peg_40;
                out_peg_42:
                goto out_peg_8;
                success_peg_40:
                ;
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_45(result_peg_3.getPosition());
                    {
                        int position_peg_47 = result_peg_45.getPosition();
                        
                        char letter_peg_48 = stream.get(result_peg_45.getPosition());
                        if (letter_peg_48 != '\0' && strchr(" \n\t", letter_peg_48) != NULL){
                            result_peg_45.nextPosition();
                            result_peg_45.setValue(Value((void*) (intptr_t) letter_peg_48));
                        } else {
                            result_peg_45.setPosition(position_peg_47);
                            goto out_peg_49;
                        }
                        
                    }
                    goto success_peg_46;
                    out_peg_49:
                    goto loop_peg_44;
                    success_peg_46:
                    ;
                    result_peg_3.addResult(result_peg_45);
                } while (true);
                loop_peg_44:
                ;
            
            
            
            {
                    int position_peg_51 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_51);
                            goto out_peg_52;
                        }
                    }
                        
                }
                goto success_peg_50;
                out_peg_52:
                goto out_peg_8;
                success_peg_50:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_rpc_decl = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_rpc_decl = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_arg_list(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_arg_list.calculated()){
        return column_peg_2.chunk4->chunk_arg_list;
    }
    
    
    RuleTrace trace_peg_1(stream, "arg_list");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_argument(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_8(result_peg_3.getPosition());
                    {
                        int position_peg_10 = result_peg_8.getPosition();
                        
                        char letter_peg_11 = stream.get(result_peg_8.getPosition());
                        if (letter_peg_11 != '\0' && strchr(" \n\t", letter_peg_11) != NULL){
                            result_peg_8.nextPosition();
                            result_peg_8.setValue(Value((void*) (intptr_t) letter_peg_11));
                        } else {
                            result_peg_8.setPosition(position_peg_10);
                            goto out_peg_12;
                        }
                        
                    }
                    goto success_peg_9;
                    out_peg_12:
                    goto loop_peg_7;
                    success_peg_9:
                    ;
                    result_peg_3.addResult(result_peg_8);
                } while (true);
                loop_peg_7:
                ;
            
            
            
            result_peg_3 = rule_arg_list_tail(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_arg_list = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_arg_list = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_arg_list_tail(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_arg_list_tail.calculated()){
        return column_peg_2.chunk4->chunk_arg_list_tail;
    }
    
    
    RuleTrace trace_peg_1(stream, "arg_list_tail");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ","));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(","[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3 = rule_arg_list(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_arg_list_tail = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
        Result result_peg_16(myposition);
        
        
        
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_arg_list_tail = result_peg_16;
        stream.update(result_peg_16.getPosition());
        
        
        return result_peg_16;
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_arg_list_tail = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_argument(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_argument.calculated()){
        return column_peg_2.chunk4->chunk_argument;
    }
    
    
    RuleTrace trace_peg_1(stream, "argument");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_simple_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_8 = result_peg_3.getPosition();
                
                result_peg_3 = rule_attributes(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_8);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_argument = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_argument = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_field(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk4 != 0 && column_peg_2.chunk4->chunk_proj_field.calculated()){
        return column_peg_2.chunk4->chunk_proj_field;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_field");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_simple_type(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            int save_peg_8 = result_peg_3.getPosition();
                
                result_peg_3 = rule_attributes(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    
                    result_peg_3 = Result(save_peg_8);
                    result_peg_3.setValue(Value((void*) 0));
                    
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_19(result_peg_3.getPosition());
                    {
                        int position_peg_21 = result_peg_19.getPosition();
                        
                        char letter_peg_22 = stream.get(result_peg_19.getPosition());
                        if (letter_peg_22 != '\0' && strchr(" \n\t", letter_peg_22) != NULL){
                            result_peg_19.nextPosition();
                            result_peg_19.setValue(Value((void*) (intptr_t) letter_peg_22));
                        } else {
                            result_peg_19.setPosition(position_peg_21);
                            goto out_peg_23;
                        }
                        
                    }
                    goto success_peg_20;
                    out_peg_23:
                    goto loop_peg_18;
                    success_peg_20:
                    ;
                    result_peg_3.addResult(result_peg_19);
                } while (true);
                loop_peg_18:
                ;
            
            
            
            {
                    int position_peg_25 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ";"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(";"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_25);
                            goto out_peg_26;
                        }
                    }
                        
                }
                goto success_peg_24;
                out_peg_26:
                goto out_peg_5;
                success_peg_24:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_proj_field = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk4 == 0){
            column_peg_2.chunk4 = new Chunk4();
        }
        column_peg_2.chunk4->chunk_proj_field = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_header(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_header.calculated()){
        return column_peg_2.chunk5->chunk_proj_header;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_header");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "proj"));
                    for (int i = 0; i < 4; i++){
                        if (compareChar("proj"[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            {
                    int position_peg_18 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "<"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("<"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_18);
                            goto out_peg_19;
                        }
                    }
                        
                }
                goto success_peg_17;
                out_peg_19:
                goto out_peg_8;
                success_peg_17:
                ;
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_22(result_peg_3.getPosition());
                    {
                        int position_peg_24 = result_peg_22.getPosition();
                        
                        char letter_peg_25 = stream.get(result_peg_22.getPosition());
                        if (letter_peg_25 != '\0' && strchr(" \n\t", letter_peg_25) != NULL){
                            result_peg_22.nextPosition();
                            result_peg_22.setValue(Value((void*) (intptr_t) letter_peg_25));
                        } else {
                            result_peg_22.setPosition(position_peg_24);
                            goto out_peg_26;
                        }
                        
                    }
                    goto success_peg_23;
                    out_peg_26:
                    goto loop_peg_21;
                    success_peg_23:
                    ;
                    result_peg_3.addResult(result_peg_22);
                } while (true);
                loop_peg_21:
                ;
            
            
            
            {
                    int position_peg_29 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "struct"));
                    for (int i = 0; i < 6; i++){
                        if (compareChar("struct"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_29);
                            goto out_peg_30;
                        }
                    }
                        
                }
                goto success_peg_28;
                out_peg_30:
                goto out_peg_8;
                success_peg_28:
                ;
            
            
            
            result_peg_3 = rule_whitespace(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_35(result_peg_3.getPosition());
                    {
                        int position_peg_37 = result_peg_35.getPosition();
                        
                        char letter_peg_38 = stream.get(result_peg_35.getPosition());
                        if (letter_peg_38 != '\0' && strchr(" \n\t", letter_peg_38) != NULL){
                            result_peg_35.nextPosition();
                            result_peg_35.setValue(Value((void*) (intptr_t) letter_peg_38));
                        } else {
                            result_peg_35.setPosition(position_peg_37);
                            goto out_peg_39;
                        }
                        
                    }
                    goto success_peg_36;
                    out_peg_39:
                    goto loop_peg_34;
                    success_peg_36:
                    ;
                    result_peg_3.addResult(result_peg_35);
                } while (true);
                loop_peg_34:
                ;
            
            
            
            {
                    int position_peg_42 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) ">"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar(">"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_42);
                            goto out_peg_43;
                        }
                    }
                        
                }
                goto success_peg_41;
                out_peg_43:
                goto out_peg_8;
                success_peg_41:
                ;
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_46(result_peg_3.getPosition());
                    {
                        int position_peg_48 = result_peg_46.getPosition();
                        
                        char letter_peg_49 = stream.get(result_peg_46.getPosition());
                        if (letter_peg_49 != '\0' && strchr(" \n\t", letter_peg_49) != NULL){
                            result_peg_46.nextPosition();
                            result_peg_46.setValue(Value((void*) (intptr_t) letter_peg_49));
                        } else {
                            result_peg_46.setPosition(position_peg_48);
                            goto out_peg_50;
                        }
                        
                    }
                    goto success_peg_47;
                    out_peg_50:
                    goto loop_peg_45;
                    success_peg_47:
                    ;
                    result_peg_3.addResult(result_peg_46);
                } while (true);
                loop_peg_45:
                ;
            
            
            
            result_peg_3 = rule_identifier(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_8;
                }
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_header = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_header = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_proj_body(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_proj_body.calculated()){
        return column_peg_2.chunk5->chunk_proj_body;
    }
    
    
    RuleTrace trace_peg_1(stream, "proj_body");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            {
                    int position_peg_6 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "{"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("{"[i], stream.get(result_peg_3.getPosition()))){
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
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_11(result_peg_3.getPosition());
                    {
                        int position_peg_13 = result_peg_11.getPosition();
                        
                        char letter_peg_14 = stream.get(result_peg_11.getPosition());
                        if (letter_peg_14 != '\0' && strchr(" \n\t", letter_peg_14) != NULL){
                            result_peg_11.nextPosition();
                            result_peg_11.setValue(Value((void*) (intptr_t) letter_peg_14));
                        } else {
                            result_peg_11.setPosition(position_peg_13);
                            goto out_peg_15;
                        }
                        
                    }
                    goto success_peg_12;
                    out_peg_15:
                    goto loop_peg_10;
                    success_peg_12:
                    ;
                    result_peg_3.addResult(result_peg_11);
                } while (true);
                loop_peg_10:
                ;
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_18(result_peg_3.getPosition());
                    {
                    
                        result_peg_18 = rule_proj_field(stream, result_peg_18.getPosition());
                            if (result_peg_18.error()){
                                goto loop_peg_17;
                            }
                        
                        
                        
                        result_peg_18.reset();
                            do{
                                Result result_peg_21(result_peg_18.getPosition());
                                {
                                    int position_peg_23 = result_peg_21.getPosition();
                                    
                                    char letter_peg_24 = stream.get(result_peg_21.getPosition());
                                    if (letter_peg_24 != '\0' && strchr(" \n\t", letter_peg_24) != NULL){
                                        result_peg_21.nextPosition();
                                        result_peg_21.setValue(Value((void*) (intptr_t) letter_peg_24));
                                    } else {
                                        result_peg_21.setPosition(position_peg_23);
                                        goto out_peg_25;
                                    }
                                    
                                }
                                goto success_peg_22;
                                out_peg_25:
                                goto loop_peg_20;
                                success_peg_22:
                                ;
                                result_peg_18.addResult(result_peg_21);
                            } while (true);
                            loop_peg_20:
                            ;
                        
                        
                    }
                    result_peg_3.addResult(result_peg_18);
                } while (true);
                loop_peg_17:
                ;
            
            
            
            {
                    int position_peg_27 = result_peg_3.getPosition();
                    
                    result_peg_3.setValue(Value((void*) "}"));
                    for (int i = 0; i < 1; i++){
                        if (compareChar("}"[i], stream.get(result_peg_3.getPosition()))){
                            result_peg_3.nextPosition();
                        } else {
                            result_peg_3.setPosition(position_peg_27);
                            goto out_peg_28;
                        }
                    }
                        
                }
                goto success_peg_26;
                out_peg_28:
                goto out_peg_8;
                success_peg_26:
                ;
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_body = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_8:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_proj_body = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_projection(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_projection.calculated()){
        return column_peg_2.chunk5->chunk_projection;
    }
    
    
    RuleTrace trace_peg_1(stream, "projection");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3 = rule_proj_header(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
            
            result_peg_3.reset();
                do{
                    Result result_peg_8(result_peg_3.getPosition());
                    {
                        int position_peg_10 = result_peg_8.getPosition();
                        
                        char letter_peg_11 = stream.get(result_peg_8.getPosition());
                        if (letter_peg_11 != '\0' && strchr(" \n\t", letter_peg_11) != NULL){
                            result_peg_8.nextPosition();
                            result_peg_8.setValue(Value((void*) (intptr_t) letter_peg_11));
                        } else {
                            result_peg_8.setPosition(position_peg_10);
                            goto out_peg_12;
                        }
                        
                    }
                    goto success_peg_9;
                    out_peg_12:
                    goto loop_peg_7;
                    success_peg_9:
                    ;
                    result_peg_3.addResult(result_peg_8);
                } while (true);
                loop_peg_7:
                ;
            
            
            
            result_peg_3 = rule_proj_body(stream, result_peg_3.getPosition());
                if (result_peg_3.error()){
                    goto out_peg_5;
                }
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_projection = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_5:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_projection = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_item(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_item.calculated()){
        return column_peg_2.chunk5->chunk_item;
    }
    
    
    RuleTrace trace_peg_1(stream, "item");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        result_peg_3 = rule_rpc_decl(stream, result_peg_3.getPosition());
        if (result_peg_3.error()){
            goto out_peg_4;
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_item = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_4:
        Result result_peg_5(myposition);
        
        
        result_peg_5 = rule_projection(stream, result_peg_5.getPosition());
        if (result_peg_5.error()){
            goto out_peg_6;
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_item = result_peg_5;
        stream.update(result_peg_5.getPosition());
        
        
        return result_peg_5;
        out_peg_6:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_item = errorResult;
        stream.update(errorResult.getPosition());
        
    
    
    return errorResult;
}
        

Result rule_file(Stream & stream, const int position){
    
    Column & column_peg_2 = stream.getColumn(position);
    if (column_peg_2.chunk5 != 0 && column_peg_2.chunk5->chunk_file.calculated()){
        return column_peg_2.chunk5->chunk_file;
    }
    
    
    RuleTrace trace_peg_1(stream, "file");
    int myposition = position;
    
    
    
    Result result_peg_3(myposition);
        
        
        {
        
            result_peg_3.reset();
                do{
                    Result result_peg_6(result_peg_3.getPosition());
                    {
                    
                        result_peg_6 = rule_item(stream, result_peg_6.getPosition());
                            if (result_peg_6.error()){
                                goto loop_peg_5;
                            }
                        
                        
                        
                        result_peg_6.reset();
                            do{
                                Result result_peg_9(result_peg_6.getPosition());
                                {
                                    int position_peg_11 = result_peg_9.getPosition();
                                    
                                    char letter_peg_12 = stream.get(result_peg_9.getPosition());
                                    if (letter_peg_12 != '\0' && strchr(" \n\t", letter_peg_12) != NULL){
                                        result_peg_9.nextPosition();
                                        result_peg_9.setValue(Value((void*) (intptr_t) letter_peg_12));
                                    } else {
                                        result_peg_9.setPosition(position_peg_11);
                                        goto out_peg_13;
                                    }
                                    
                                }
                                goto success_peg_10;
                                out_peg_13:
                                goto loop_peg_8;
                                success_peg_10:
                                ;
                                result_peg_6.addResult(result_peg_9);
                            } while (true);
                            loop_peg_8:
                            ;
                        
                        
                    }
                    result_peg_3.addResult(result_peg_6);
                } while (true);
                loop_peg_5:
                if (result_peg_3.matches() == 0){
                    goto out_peg_14;
                }
            
            
            
            if ('\0' == stream.get(result_peg_3.getPosition())){
                    result_peg_3.nextPosition();
                    result_peg_3.setValue(Value((void *) '\0'));
                } else {
                    goto out_peg_14;
                }
            
            
        }
        
        
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_file = result_peg_3;
        stream.update(result_peg_3.getPosition());
        
        
        return result_peg_3;
        out_peg_14:
    
        if (column_peg_2.chunk5 == 0){
            column_peg_2.chunk5 = new Chunk5();
        }
        column_peg_2.chunk5->chunk_file = errorResult;
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

    