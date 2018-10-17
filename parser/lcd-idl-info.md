## `lcd_idl.cpp` info 
This file contains comments on the generated `lcd_idl.cpp`
file by the vembyr parser generator after being executed with the
 `lcd_idl.peg` grammar file as input.

In the `lcd_idl.cpp` parser, each rule function has the following format:
```
rule_<rulename>(Stream & stream, const position) 
```

- Includes are stored as follows:
```
result_peg_8 = rule_Include(stream, result_peg_8.getPosition());
```
- they are in `result_peg_8`,

which is then taken in `Values`..

```
i = result_peg_3.getValues();
```


