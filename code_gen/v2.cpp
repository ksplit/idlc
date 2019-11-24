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

  auto get_type_spec(Type* t) {
    switch (t->num()) {
    case INTEGER_TYPE:
      switch (dynamic_cast<IntegerType*>(t)->int_type()) {
      case pt_int_t:
        return CCSTSimpleTypeSpecifier::IntegerTypeSpec;

      case pt_long_t:
        return CCSTSimpleTypeSpecifier::LongTypeSpec;

      case pt_short_t:
        return CCSTSimpleTypeSpecifier::ShortTypeSpec;

      case pt_char_t:
        return CCSTSimpleTypeSpecifier::CharTypeSpec;
      }

    case VOID_TYPE:
      return CCSTSimpleTypeSpecifier::VoidTypeSpec;

    default:
      return CCSTSimpleTypeSpecifier::OtherTypeSpec;
    }
  }

  void generate_callee_protos(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto proto_name = rpc->name();
        const auto type_spec = get_type_spec(rpc->return_variable()->type());

        std::vector<CCSTParamDeclaration*> params;
        for (const auto prm : *rpc) {
          const auto pts = get_type_spec(prm->type());
          params.push_back(
            new CCSTParamDeclaration(
              {new CCSTSimpleTypeSpecifier(pts)},
              new CCSTDeclarator(
                nullptr,
                new CCSTDirectDecId(prm->identifier())
              )
            )
          );
        }

        const auto f_proto = new CCSTDirectDecParamTypeList(
          new CCSTDirectDecId(proto_name),
          new CCSTParamList(params)
        );
        decls.push_back(new CCSTDeclaration(
          {new CCSTSimpleTypeSpecifier(type_spec)},
          {new CCSTInitDeclarator(new CCSTDeclarator(nullptr, f_proto))}
        ));
      }
    }
  }

  CCSTCompoundStatement* generate_rpc_marshal(Rpc* rpc)
  {
    const auto rpc_id = new CCSTPrimaryExprId(rpc->name());
    const auto rpc_call = new CCSTPostFixExprAssnExpr(rpc_id, {});
    auto rpcs = new CCSTExprStatement(rpc_call);

    const auto rt = rpc->return_variable()->type();
    if (rt->num() != VOID_TYPE) {
      const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
      const auto rreg = new CCSTPostFixExprExpr(regs, new CCSTInteger(0));
      rpcs = new CCSTExprStatement(new CCSTAssignExpr(rreg, new CCSTAssignOp(equal_t), rpc_call));
    }

    return new CCSTCompoundStatement({}, {rpcs});
  }

  void generate_marshal_funcs(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto def_name = rpc->callee_name();
        std::vector<CCSTParamDeclaration*> params {
          new CCSTParamDeclaration(
            {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
            new CCSTDeclarator(
              new CCSTPointer(),
              new CCSTDirectDecId("msg")
            )
          )
        };
        
        const auto f_proto = new CCSTDirectDecParamTypeList(
          new CCSTDirectDecId(def_name),
          new CCSTParamList(params)
        );
        decls.push_back(new CCSTFuncDef(
          {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec)},
          new CCSTDeclarator(nullptr, f_proto),
          {},
          generate_rpc_marshal(rpc)
        ));
      }
    }
  }

  CCSTFile* generate_klcd_impl(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;

    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_klcd.h", true));
    decls.push_back(new CCSTPreprocessor("stdio.h", false));
    decls.push_back(new CCSTPreprocessor("stdint.h", false));
    generate_callee_protos(p, decls);
    generate_marshal_funcs(p, decls);

    return new CCSTFile(decls);
  }
}