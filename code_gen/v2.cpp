#include "code_gen.h"

namespace v2 {
  CCSTFile* generate_common_header(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls {};

    const auto idl_h = new Include(false, "ipc.h");
    decls.push_back(new CCSTPreprocessor(idl_h->get_path(), idl_h->is_relative()));

    decls.push_back(new CCSTDeclaration(
      {new CCSTStructUnionSpecifier(struct_t, "channel")},
      {new CCSTInitDeclarator(
        new CCSTDeclarator(
          nullptr,
          new CCSTDirectDecId(p->main_module()->id() + "_ch")
        )
      )}
    ));

    std::vector<CCSTEnumerator*> rpcs;
    for (const auto m : *p)
      for (const auto rpc : *m)
        rpcs.push_back(new CCSTEnumerator("KLCD_" + rpc->enum_name()));
    decls.push_back(
      new CCSTDeclaration(
        {new CCSTEnumSpecifier("dispatch_t", new CCSTEnumeratorList(new decltype(rpcs)(rpcs)))},
        {}
      )
    );

    return new CCSTFile(decls);
  }
  
  CCSTFile* generate_klcd_header(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;
    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_common.h", true));

    const auto msg_struct_type = new CCSTStructUnionSpecifier(struct_t, "fipc_message");
    const auto msg_p = new CCSTParamDeclaration(
      {msg_struct_type},
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("msg"))
    );
    const auto dispatch_f = new CCSTDirectDecParamTypeList(
      new CCSTDirectDecId("dispatch_klcd"),
      new CCSTParamList({msg_p})
    );
    decls.push_back(new CCSTDeclaration(
      {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec)},
      {new CCSTInitDeclarator(new CCSTDeclarator(nullptr, dispatch_f))}
    ));

    return new CCSTFile(decls);
  }

  // Forward-declare the functions declared for the KLCD side
  void generate_callee_protos(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p)
      for (const auto rpc : *m)
        decls.push_back(function_declaration(rpc));
  }

  // Generates all the marshaling/call code for a *_callee function
  CCSTCompoundStatement* generate_rpc_callee(Rpc* rpc)
  {
    const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
    
    std::vector<CCSTDeclaration*> decls;
    std::vector<CCSTAssignExpr*> params;
    for (const auto prm : *rpc) {
      const auto name = prm->identifier();
      const auto type_spec = type2(prm->type());
      const auto reg = new CCSTPostFixExprExpr(regs, new CCSTInteger(prm->marshal_info()->get_register()));
      decls.push_back(new CCSTDeclaration(
        type_spec,
        {new CCSTInitDeclarator(
          new CCSTDeclarator(
            nullptr,
            new CCSTDirectDecId(name)
          ),
          new CCSTInitializer(reg)
        )}
      ));
      params.push_back(new CCSTPrimaryExprId(name));
    }

    const auto rpc_id = new CCSTPrimaryExprId(rpc->name());
    const auto rpc_call = new CCSTPostFixExprAssnExpr(rpc_id, params);
    auto rpcs = new CCSTExprStatement(rpc_call);
    function_call(rpc->name(), params);

    const auto rt = rpc->return_variable()->type();
    if (rt->num() != VOID_TYPE) {
      const auto rreg = new CCSTPostFixExprExpr(regs, new CCSTInteger(0));
      rpcs = new CCSTExprStatement(new CCSTAssignExpr(rreg, new CCSTAssignOp(equal_t), rpc_call));
    }

    return new CCSTCompoundStatement(decls, {rpcs});
  }

  // Replaces function_declaration for marshaling wrappers
  CCSTDeclaration* make_rpc_declaration(Rpc* rpc)
  {
    const auto ret_type = type2(rpc->return_variable()->type());
    const auto stars = pointer(rpc->return_variable()->pointer_count());
    const auto id = new CCSTDirectDecId(rpc->callee_name());
    const auto param = new CCSTParamDeclaration(
      {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
      new CCSTDeclarator(
        new CCSTPointer(),
        new CCSTDirectDecId("msg")
      )
    );

    const auto plist = new CCSTDirectDecParamTypeList(id, new CCSTParamList({param}));
    const auto decl = new CCSTDeclarator(stars, plist);

    return new CCSTDeclaration(ret_type, {decl});
  }

  // Do the above code generation for each rpc (in the KLCD), and of course, wrap it in a function definition
  void generate_marshal_funcs(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto fn_dec = make_rpc_declaration(rpc);
        const auto fn_def = function_definition(fn_dec, generate_rpc_callee(rpc));
        decls.push_back(fn_def);
      }
    }
  }

  CCSTFile* generate_klcd_impl(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;

    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_klcd.h", true));
    generate_callee_protos(p, decls);
    generate_marshal_funcs(p, decls);
    generate_klcd_dispatch(p, decls);

    return new CCSTFile(decls);
  }

  void generate_klcd_dispatch(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    const auto def_case = new CCSTDefaultLabelStatement(new CCSTReturn(new CCSTInteger(1)));
    std::vector<CCSTStatement*> cases {def_case};

    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto rpc_id = new CCSTPrimaryExprId(rpc->callee_name());
        const auto rpc_call = new CCSTPostFixExprAssnExpr(rpc_id, {new CCSTPrimaryExprId("msg")});
        auto rpcs = new CCSTExprStatement(rpc_call);
        cases.push_back(new CCSTCaseStatement(new CCSTPrimaryExprId("KLCD_" + rpc->enum_name()), {rpcs}));
      }
    }

    const auto switch_body = new CCSTCompoundStatement({}, cases);
    const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
    const auto rreg = new CCSTPostFixExprExpr(regs, new CCSTInteger(0));
    const auto swtch = new CCSTSwitchStatement(rreg, switch_body);
    const auto final_ret = new CCSTReturn(new CCSTInteger(0));
    const auto body = new CCSTCompoundStatement({}, {swtch, final_ret});

    const std::vector<CCSTParamDeclaration*> params {
      new CCSTParamDeclaration(
        {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
        new CCSTDeclarator(
          new CCSTPointer(),
          new CCSTDirectDecId("msg")
        )
      )
    };
    const auto f_proto = new CCSTDirectDecParamTypeList(
      new CCSTDirectDecId("dispatch_klcd"),
      new CCSTParamList(params)
    );
    decls.push_back(new CCSTFuncDef(
      {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec)},
      new CCSTDeclarator(nullptr, f_proto),
      {},
      body
    ));
  }

  void generate_lcd_dispatch(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    const auto def_case = new CCSTDefaultLabelStatement(new CCSTReturn(new CCSTInteger(1)));
    std::vector<CCSTStatement*> cases {def_case};

    for (const auto m : *p) {
      for (const auto rpc : *m) {
        /// TODO: Likely need a total rewrite of how "side" is handled for marshaling
        // As opposed to the current system of "is it an fptr."
        if (rpc->function_pointer_defined()) {
          const auto rpc_id = new CCSTPrimaryExprId(rpc->callee_name());
          const auto rpc_call = new CCSTPostFixExprAssnExpr(rpc_id, {new CCSTPrimaryExprId("msg")});
          auto rpcs = new CCSTExprStatement(rpc_call);
          cases.push_back(new CCSTCaseStatement(new CCSTPrimaryExprId("KLCD_" + rpc->enum_name()), {rpcs}));
        }
      }
    }

    const auto switch_body = new CCSTCompoundStatement({}, cases);
    const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
    const auto rreg = new CCSTPostFixExprExpr(regs, new CCSTInteger(0));
    const auto swtch = new CCSTSwitchStatement(rreg, switch_body);
    const auto final_ret = new CCSTReturn(new CCSTInteger(0));
    const auto body = new CCSTCompoundStatement({}, {swtch, final_ret});

    const std::vector<CCSTParamDeclaration*> params {
      new CCSTParamDeclaration(
        {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
        new CCSTDeclarator(
          new CCSTPointer(),
          new CCSTDirectDecId("msg")
        )
      )
    };
    const auto f_proto = new CCSTDirectDecParamTypeList(
      new CCSTDirectDecId("dispatch_lcd"),
      new CCSTParamList(params)
    );
    decls.push_back(new CCSTFuncDef(
      {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec)},
      new CCSTDeclarator(nullptr, f_proto),
      {},
      body
    ));
  }

  CCSTDeclaration* rpc_function_declaration(Rpc* r)
  {
    std::vector<CCSTDecSpecifier *> specifier =
      type2(r->return_variable()->type());

    std::vector<CCSTInitDeclarator *> func;
    CCSTPointer *p = pointer(r->return_variable()->pointer_count());

    CCSTDirectDecId *name = new CCSTDirectDecId(r->name() + "_rpc");
    CCSTParamTypeList *param_list = parameter_list(r->parameters());

    CCSTDirectDecParamTypeList *name_params =
        new CCSTDirectDecParamTypeList(name, param_list);

    func.push_back(new CCSTDeclarator(p, name_params));

    return new CCSTDeclaration(specifier, func);
  }

  CCSTCompoundStatement* generate_rpc_caller(Rpc* rpc, const std::string& ch_name)
  {
    const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
    
    std::vector<CCSTDeclaration*> decls;
    std::vector<CCSTStatement*> s;
    std::vector<CCSTAssignExpr*> params;

    decls.push_back(new CCSTDeclaration(
      {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
      {new CCSTDeclarator(
        nullptr,
        new CCSTDirectDecId("msg")
      )}
    ));

    params.reserve(2);
    params.push_back(new CCSTUnaryExprCastExpr(new CCSTUnaryOp(unary_bit_and_t), new CCSTPrimaryExprId(ch_name)));
    params.push_back(new CCSTUnaryExprCastExpr(new CCSTUnaryOp(unary_bit_and_t), new CCSTPrimaryExprId("msg")));

    s.push_back(
      new CCSTExprStatement(
        new CCSTAssignExpr(
          new CCSTPostFixExprExpr(regs, new CCSTInteger(0)),
          new CCSTAssignOp(equal_t),
          new CCSTPrimaryExprId(rpc->enum_name())
        )
      )
    );
    for (const auto prm : *rpc) {
      const auto name = prm->identifier();
      const auto type_spec = type2(prm->type());
      const auto reg = new CCSTPostFixExprExpr(regs, new CCSTInteger(prm->marshal_info()->get_register()));
      s.push_back(
        new CCSTExprStatement(
          new CCSTAssignExpr(
            reg,
            new CCSTAssignOp(equal_t),
            new CCSTPrimaryExprId(prm->identifier())
          )
        )
      );
    }

    const auto rpc_call = function_call("send", params);
    auto rpcs = new CCSTExprStatement(rpc_call);
    s.push_back(rpcs);

    const auto rt = rpc->return_variable()->type();
    if (rt->num() != VOID_TYPE)
      s.push_back(new CCSTReturn(new CCSTPostFixExprExpr(regs, new CCSTInteger(0))));

    return new CCSTCompoundStatement(decls, s);
  }

  void generate_lcd_marshal(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    const auto ch_name = p->main_module()->id() + "_ch";
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto fn_dec = rpc_function_declaration(rpc);
        const auto fn_def = function_definition(fn_dec, generate_rpc_caller(rpc, ch_name));
        decls.push_back(fn_def);
      }
    }
  }

  CCSTFile* generate_lcd_impl(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;
    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_lcd.h", true));
    generate_lcd_dispatch(p, decls);
    generate_lcd_marshal(p, decls);
    return new CCSTFile(decls);
  }
}