


## Nonterminal Dependencies

Here we show dependencies of nonterminal symbols
based on the current peg grammar. Tags are added in the dependency 
tree the meanings of which are described below: 

- To see all the dependencies of a specific nonterminal, look at the entry of that nonterminal, where its name ends with a `*define`, e.g. `Include*define`. 
- If a nonterminal symbol ends with `*none`, it depends on no other nonterminal symbols. In other words, it expands to terminal symbols when its grammar rule is used.
- If a nonterminal symbol ends with `*derived`, it has already been derived in this file.
- If a nonterminal symbol does not end with anything, it is yet to be defined in this file.

- Meanwhile, this can be useful to find out the rule for a specific non terminal. 

```
grep -n "Space.*=" lcd_idl.peg 
```

[TODO] It would be great if this dependency tree can be automatically generated.

```
File*define
-|Include*define
--|Pathname
--|Spacing
--|Space*define
---|EndOfLine*derived
---|Comment*derived
---|Line_comment*derived
-|Interface
-|GlobalScopeDefinitions*define
--|Type_Definitions*define
---|Typedef
---|Projection
---|Projection_constructor*define
----|Projection_constructor_special
----|Spacing*define
-----|Space*define
------|EndOfLine*define
------|Line_comment*define
-------|Line_comm_start*none
-------|Line_comm_rest*define
--------|Line_comm_end*none
--------|Line_comm_rest*derived
------|Comment*define
-------|Comment_start*none
-------|Comment_rest*define
--------|Comment_end*none
--------|Comment_rest*derived
```

