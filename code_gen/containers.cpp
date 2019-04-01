#include "ccst.h"
#include "code_gen.h"
#include "lcd_ast.h"

/*
 * code related to container structures.
 */

std::string generated_src;

std::vector<CCSTAssignExpr *> container_of_args(CCSTPostFixExpr *struct_pointer,
                                                const std::string &type_name,
                                                const std::string &field_name) {
  std::vector<CCSTAssignExpr *> args;
  args.push_back(struct_pointer);
  args.push_back(new CCSTPrimaryExprId(type_name));
  args.push_back(new CCSTPrimaryExprId(field_name));
  return args;
}
// ah note - the following function returns the container of a variable for
// which a container has already been assigned.
std::vector<CCSTStatement *> container_of(Variable *v,
                                          const std::string &cspace) {
  std::vector<CCSTStatement *> statements;

  statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
      new CCSTPrimaryExprId(v->container()->identifier()), equals(),
      function_call("container_of",
                    container_of_args(
                        access(v), struct_name(v->container()->type()->name()),
                        v->type()->name())))));

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast failed\n");

    for (auto pf : *pt) {
      if (pf->container() != 0x0) {
        std::vector<CCSTStatement *> tmp = container_of(pf, cspace);
        statements.insert(statements.end(), tmp.begin(), tmp.end());
      }
    }
  }

  if (v->alloc_callee()) {
    ProjectionType *container =
        dynamic_cast<ProjectionType *>(v->container()->type());
    Assert(container != 0x0,
           "Error: variables's container does not have type projection\n");

    ProjectionField *my_ref_field = container->get_field("my_ref");
    Assert(my_ref_field != 0x0,
           "Error: could not find my_ref field in projection\n");

    std::vector<CCSTAssignExpr *> insert_args;
    insert_args.push_back(new CCSTPrimaryExprId(cspace));
    insert_args.push_back(new CCSTPrimaryExprId(v->container()->identifier()));
    insert_args.push_back(new CCSTUnaryExprCastExpr(
        reference(), access(my_ref_field))); // & container->my_ref

    if (v->type()->num() == FUNCTION_TYPE) {
      Function *f = dynamic_cast<Function *>(v->type());
      Assert(f != 0x0, "Error: dynamic cast to projection type failed.\n");
      statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
          new CCSTPrimaryExprId("ret"), equals(),
          function_call(insert_name(f->name()) + "_type", insert_args))));
    } else {
      ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed.\n");

      statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
          new CCSTPrimaryExprId("ret"), equals(),
          function_call(insert_name(pt->real_type() + "_type"), insert_args))));
    }
    std::string *goto_ins = new std::string("fail_insert");
    /* do error checking */
    statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret"),
                                           "lcd insert", *goto_ins));
  }

  return statements;
}

CCSTCompoundStatement *set_remote_ref(Variable *v, Channel::ChannelType type,
                                      std::string from) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;

  Assert(v->container() != 0x0,
         "Error: container is null. this should never happen\n");

  // this containers remote ref = remote ref

  ProjectionType *pt = dynamic_cast<ProjectionType *>(v->container()->type());
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

  ProjectionField *my_ref_field = pt->get_field("my_ref");
  Assert(my_ref_field != 0x0,
         "Error: could not find my_ref field in structure\n");

  ProjectionType *my_cptr_t =
      dynamic_cast<ProjectionType *>(my_ref_field->type());
  Assert(my_cptr_t != 0x0, "Error: dynamic cast to projection type failed\n");

  ProjectionField *my_cptr = my_cptr_t->get_field("cptr");
  Assert(my_cptr != 0x0, "Error: could not find cptr field\n");

  ProjectionField *other_ref_field = pt->get_field("other_ref");
  Assert(other_ref_field != 0x0,
         "Error: could not find other_ref field in structure\n");

  ProjectionType *other_cptr_t =
      dynamic_cast<ProjectionType *>(other_ref_field->type());
  Assert(other_cptr_t != 0x0,
         "Error: dynamic cast to projection type failed\n");

  ProjectionField *other_cptr = other_cptr_t->get_field("cptr");
  Assert(other_cptr != 0x0, "Error: could not find cptr field\n");

  std::string fipc_var;
  if (my_cptr->marshal_info() != 0x0) {
    if (from == "caller") {
      fipc_var.append("_response");
    } else if (from == "callee") {
      fipc_var.append("_request");
    }

    statements.push_back(new CCSTExprStatement(
        new CCSTAssignExpr(access(other_cptr), equals(),
                           unmarshal_variable(my_cptr, type, fipc_var))));
  }

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement *
allocate_and_link_containers_callee(Variable *v, const std::string &cspace,
                                    Channel::ChannelType type) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;

  // when do we want to allocate and when do we want to lookup
  if (v->container()) {
    if (v->alloc_callee()) {
      statements.push_back(alloc_insert_variable_container(v, cspace, true));
      // store remote reference;
      statements.push_back(set_remote_ref(v, type, "callee"));
    } else { // lookup
      statements.push_back(lookup_variable_container(v, type));
    }
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    auto *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      /// If the top level structure is marked dealloc then all projection
      /// fields must also be deallocated after the RPC is done. So, it's dumb
      /// to allocate memory if the projection fields are marked alloc.
      if (v->dealloc_callee()) {
        pf->set_alloc_callee(false);
        pf->set_dealloc_callee(true);
        statements.push_back(
            allocate_and_link_containers_callee(pf, cspace, type));
      }
      // link
      if (v->container() != 0x0 && pf->container() != 0x0) {
        if (v->alloc_callee() || pf->alloc_callee()) {
          /// if container is marked alloc, insert container as well
          statements.push_back(
              alloc_insert_variable_container(pf, cspace, true));
          // container->real_field.pf = pf_container->real_field;
          auto *v_container_type =
              dynamic_cast<ProjectionType *>(v->container()->type());
          Assert(v_container_type != 0x0,
                 "Error: dynamic cast to projection type failed\n");

          auto *tmp_real_field =
              find_field(v_container_type, v->type()->name());
          auto *tmp_this_pf =
              find_field(dynamic_cast<ProjectionType *>(tmp_real_field->type()),
                         pf->identifier());

          auto *container_pf =
              dynamic_cast<ProjectionType *>(pf->container()->type());
          Assert(container_pf != 0x0,
                 "Error: dynamic cast to projection type failed\n");

          statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
              access(tmp_this_pf), equals(),
              access(find_field(container_pf, pf->type()->name())))));
        }
      }
    }
  }
  return new CCSTCompoundStatement(declarations, statements);
}

/* has already been declared at v->container()->identifier() */
CCSTStatement *lookup_variable_container(Variable *v,
                                         Channel::ChannelType type) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;

  Assert(v->container() != 0x0, "Error: variable has no container\n");

  std::vector<CCSTAssignExpr *> lookup_args;
  lookup_args.push_back(new CCSTPrimaryExprId("cspace")); // lookup args 1

  ProjectionType *container =
      dynamic_cast<ProjectionType *>(v->container()->type());
  Assert(container != 0x0,
         "Error: variables's container does not have type projection\n");
  ProjectionField *ref_field;

  /// Depending on the specification, pick the reference field
  if ((v->bind_caller() && v->bind_callee()) || v->dealloc_callee()) {
    ref_field = container->get_field("other_ref");
  } else {
    ref_field = container->get_field("my_ref");
  }

  Assert(ref_field != 0x0,
         "Error: could not find my_ref field in projection\n");
  std::vector<CCSTAssignExpr *> __cptr_args;
  __cptr_args.push_back(
      unmarshal_variable(get_cptr_field(ref_field), type, "_request"));

  lookup_args.push_back(function_call(
      "__cptr", __cptr_args)); // ref we get from __cptr(reg_whatever())
  lookup_args.push_back(new CCSTUnaryExprCastExpr(
      reference(), new CCSTPrimaryExprId(v->container()->identifier())));

  ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
  Assert(pt != 0x0, "Error: dynamic cast failed.\n");

  statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
      new CCSTPrimaryExprId("ret"), equals(),
      function_call(lookup_name(pt->real_type()) + +"_type", lookup_args))));

  std::string *goto_lookup = new std::string("fail_lookup");
  /* do error checking */
  statements.push_back(
      if_cond_fail_goto(new CCSTPrimaryExprId("ret"), "lookup", *goto_lookup));

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement *alloc_variable(Variable *v) {
  AllocateTypeVisitor *worker = new AllocateTypeVisitor();
  return v->type()->accept(worker, v);
}

CCSTStatement *allocate_non_container_variables(Variable *v) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  if (v->container() == 0x0) {
    if (v->pointer_count() > 0) {
      return alloc_variable(v);
    }

    if (v->type()->num() == PROJECTION_TYPE ||
        v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

      for (auto pf : *pt) {

        // if pf is a pointer need to allocate.
        if (pf->container() == 0x0 && pf->pointer_count() > 0) {
          statements.push_back(allocate_non_container_variables(pf));
        } else if (pf->container() != 0x0) {
          // need to link if it has a container

          // get the fields container
          ProjectionType *pf_container_type =
              dynamic_cast<ProjectionType *>(pf->container()->type());
          Assert(pf_container_type != 0x0,
                 "Error: dynamic cast to projection type failed\n");

          ProjectionField *container_pf_field =
              find_field(pf_container_type, pf->type()->name());

          statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
              access(pf), equals(), access(container_pf_field))));
        }
      }
    }
  } else if (v->type()->num() == PROJECTION_TYPE ||
             v->type()->num() ==
                 PROJECTION_CONSTRUCTOR_TYPE) { // may need to allocate some
                                                // fields
    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      // v is a container, so if pf is also a container they ahve already been
      // allocated and linked otherwise we want to allocate pf and link them.
      if (pf->container() == 0x0 && pf->pointer_count() > 0) {

        // want to allocate the containers corresponding field.
        ProjectionType *v_container_type =
            dynamic_cast<ProjectionType *>(v->container()->type());
        Assert(v_container_type != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *tmp_real_field =
            find_field(v_container_type, pt->real_type());

        ProjectionField *container_pf =
            find_field(dynamic_cast<ProjectionType *>(tmp_real_field->type()),
                       pf->identifier());

        statements.push_back(allocate_non_container_variables(container_pf));
      }
    }
  }

  return new CCSTCompoundStatement(declarations, statements);
}

// Called by server.cpp
CCSTCompoundStatement *insert_variable_container(Variable *v,
                                                 const std::string &cspace) {
  std::string callee_src("callee");
  generated_src = callee_src;
  return alloc_insert_variable_container(v, cspace, false);
}

/* this function allocates the variable's container.
 * assumes the container has already been declared at
 * v->container()->identifier() inserts container into cspace
 */
CCSTCompoundStatement *
alloc_insert_variable_container(Variable *v, const std::string &cspace,
                                bool alloc) {
  // TODO: set other ref;
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;

  /// Allocate an instance of the container
  if (alloc) {
    /// we only ever allocate containers
    Assert(v->container() != 0x0, "Error: variable has no container\n");

    /// kzalloc structure
    std::vector<CCSTAssignExpr *> kzalloc_args;

    auto *worker = new TypeNameVisitor();
    auto *type_name = v->container()->type()->accept(worker);
    delete worker;
    // For allocating memory for struct foo *foo1;
    // Use sizeof(struct foo) instead of sizeof(foo1)
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(type_name));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

    /* this is ok because container is always a single pointer. */
    statements.push_back(new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId(v->container()->identifier()),
                           equals(), function_call("kzalloc", kzalloc_args))));

    auto *goto_alloc = new std::string("fail_alloc");
    /* do error checking */
    statements.push_back(if_cond_fail_goto(
        new CCSTUnaryExprCastExpr(
            Not(), new CCSTPrimaryExprId(v->container()->identifier())),
        "kzalloc", *goto_alloc));
  }

  /// insert container into cspace
  /// find my_ref field
  auto *container = dynamic_cast<ProjectionType *>(v->container()->type());
  Assert(container != 0x0,
         "Error: variables's container does not have type projection\n");

  auto *my_ref_field = container->get_field("my_ref");
  Assert(my_ref_field != 0x0,
         "Error: could not find my_ref field in projection\n");

  std::vector<CCSTAssignExpr *> insert_args;
  insert_args.push_back(new CCSTPrimaryExprId(cspace));
  insert_args.push_back(new CCSTPrimaryExprId(v->container()->identifier()));
  insert_args.push_back(new CCSTUnaryExprCastExpr(
      reference(), access(my_ref_field))); // & container->my_ref

  if (v->type()->num() == FUNCTION_TYPE) {
    auto *f = dynamic_cast<Function *>(v->type());
    Assert(f != NULL, "Error: dynamic cast to function type failed.\n");
    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
        new CCSTPrimaryExprId("ret"), equals(),
        function_call(insert_name(f->name()) + "_type", insert_args))));
  } else {
    auto *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != NULL, "Error: c cast to projection type failed.\n");

    if (generated_src.compare("caller") == 0) {
      auto *pt_container =
          dynamic_cast<ProjectionType *>(v->container()->type());
      ProjectionField *content = pt_container->get_field(pt->real_type());
      Assert(content != 0x0,
             "Error: could not find structure field in container\n");
      statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
          new CCSTPrimaryExprId(v->identifier()), equals(),
          new CCSTUnaryExprCastExpr(reference(), access(content)))));
    }

    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
        new CCSTPrimaryExprId("ret"), equals(),
        function_call(insert_name(pt->real_type()) + "_type", insert_args))));
  }

  auto *goto_insert = new std::string("fail_insert");
  /// do error checking
  statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret"),
                                         "lcd insert", *goto_insert));

  return new CCSTCompoundStatement(declarations, statements);
}

/* this function's verification isn't very good */
ProjectionField *get_cptr_field(Variable *v) {
  ProjectionType *cptr_t = dynamic_cast<ProjectionType *>(v->type());
  Assert(cptr_t != 0x0, "Error: dynamic cast to projection type failed.\n");

  ProjectionField *cptr = cptr_t->get_field("cptr");
  Assert(cptr != 0x0, "Error: could not find field cptr.\n");

  return cptr;
}

ProjectionField *find_field(ProjectionType *pt, const std::string &field_name) {
  ProjectionField *field = pt->get_field(field_name);
  Assert(field != 0x0, "Error: could not find field %s\n", field_name.c_str());
  return field;
}

// Called by client.cpp
CCSTCompoundStatement *
alloc_link_container_caller_top(Variable *v, const std::string &cspace) {
  std::string caller_src("caller");
  generated_src = caller_src;
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  // simple case. v is not a projection
  if (v->type()->num() != PROJECTION_TYPE &&
      v->type()->num() != FUNCTION_TYPE) {
    if (v->alloc_caller()) {
      return alloc_insert_variable_container(v, cspace, true);
    } else {
      return new CCSTCompoundStatement(declarations, container_of(v, cspace));
    }
  }

  if (v->alloc_caller()) {
    // else it is the projection case.
    statements.push_back(alloc_insert_variable_container(v, cspace, true));
  }
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement *alloc_link_container_caller(Variable *v,
                                                   const std::string &cspace) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  // simple case. v is not a projection
  if (v->type()->num() != PROJECTION_TYPE &&
      v->type()->num() != FUNCTION_TYPE) {
    if (v->alloc_caller()) {
      return alloc_insert_variable_container(v, cspace, true);
    } else {
      return new CCSTCompoundStatement(declarations, container_of(v, cspace));
    }
  }

  if (v->alloc_caller()) {
    // else it is the projection case.
    statements.push_back(alloc_insert_variable_container(v, cspace, true));

    if (v->type()->num() == FUNCTION_TYPE)
      goto done;
    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      // auto *ptype = dynamic_cast<ProjectionType*>(v->type());
      ProjectionType *v_container_type =
          dynamic_cast<ProjectionType *>(v->container()->type());
      Assert(v_container_type != 0x0,
             "Error: dynamic cast to projection type failed\n");

      ProjectionType *tmp = dynamic_cast<ProjectionType *>(
          find_field(v_container_type, v->type()->name())->type());
      //								      ,
      // ptype->real_type())->type());
      ProjectionField *tmp_field = find_field(tmp, pf->identifier());
      if (pf->container() != 0x0) {
        statements.push_back(alloc_link_container_caller(pf, cspace));
        // link
        ProjectionType *container_pf =
            dynamic_cast<ProjectionType *>(pf->container()->type());
        Assert(container_pf != 0x0,
               "Error: dynamic cast to projection type failed\n");

        statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
            access(tmp_field), equals(),
            access(find_field(container_pf, pf->type()->name())))));

      } else if (pf->pointer_count() > 0) {
        // allocate and link
        // declare at a tmp name
        ProjectionField *tmp_v = new ProjectionField(
            pf->type(), append_strings("_", construct_list_vars(pf)),
            pf->pointer_count());
        declarations.push_back(declare_variable(tmp_v));
        statements.push_back(alloc_variable(tmp_v));
        statements.push_back(new CCSTExprStatement(
            new CCSTAssignExpr(access(tmp_field), equals(),
                               new CCSTPrimaryExprId(tmp_v->identifier()))));
      }
    }
  } else {
    return new CCSTCompoundStatement(declarations, container_of(v, cspace));
  }
done:
  return new CCSTCompoundStatement(declarations, statements);
}

// code to allocate channels
std::vector<CCSTStatement *> caller_allocate_channels(ProjectionType *pt) {
  std::vector<CCSTStatement *> statements;

  for (auto pf : pt->channels_) {
    if (pf->alloc_caller()) { // allocate it.

      // ret = lcd_create_sync_endpoint(&fs_container->chnl);
      std::vector<CCSTAssignExpr *> lcd_create_sync_endpoint_args;
      lcd_create_sync_endpoint_args.push_back(
          new CCSTUnaryExprCastExpr(reference(), access(pf)));

      statements.push_back(new CCSTExprStatement(
          new CCSTAssignExpr(new CCSTPrimaryExprId("ret"), equals(),
                             function_call("lcd_create_sync_endpoint",
                                           lcd_create_sync_endpoint_args))));

      // error checking
      statements.push_back(if_cond_fail(new CCSTPrimaryExprId("ret"),
                                        "lcd_create_sync_endpoint"));
    }
  }

  // recurse on fields.
  for (auto pf : *pt) {
    if (pf->type_->num() == PROJECTION_TYPE ||
        pf->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      ProjectionType *tmp = dynamic_cast<ProjectionType *>(pf->type_);
      Assert(tmp != 0x0, "Error: dynamic cast to projection type failed\n");

      std::vector<CCSTStatement *> tmp_statements =
          caller_allocate_channels(tmp);
      statements.insert(statements.end(), tmp_statements.begin(),
                        tmp_statements.end());
    }
  }

  return statements;
}

std::vector<CCSTStatement *> caller_initialize_channels(ProjectionType *pt) {
  std::vector<CCSTStatement *> statements;

  if (pt->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionConstructorType *pct =
        dynamic_cast<ProjectionConstructorType *>(pt);
    Assert(pct != 0x0,
           "Error: dynamic cast to projection constructor type failed\n");

    for (auto pair : pct->channel_params_) {
      if (pair.second == 0x0) {
        std::cout << "pair.second is null, pair.first id is "
                  << pair.first->identifier() << std::endl;
      }
      /*
        lcd_sync_channel_group_item_init(&fs_operations_container->chnl,
        fs_container->chnl, 0,
        dispatch_minix_channel);
      */
      std::vector<CCSTAssignExpr *> args;
      args.push_back(new CCSTUnaryExprCastExpr(
          reference(), access(pair.first))); // what we are initializing
      args.push_back(access(pair.second));   // what we are initializing it to
      args.push_back(new CCSTInteger(0)); // expected c_ptrs how to determine?
      args.push_back(
          new CCSTPrimaryExprId("function pointer todo")); // function pointer

      statements.push_back(new CCSTExprStatement(
          function_call("lcd_sync_channel_group_item_init", args)));
    }
  }

  // recurse on fields.
  for (auto pf : *pt) {
    if ((pf->type_->num() == PROJECTION_TYPE ||
         pf->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) &&
        pf->alloc_caller()) {
      ProjectionType *tmp = dynamic_cast<ProjectionType *>(pf->type_);
      Assert(tmp != 0x0, "Error: dynamic cast to projection type failed\n");

      std::vector<CCSTStatement *> tmp_statements =
          caller_initialize_channels(tmp);
      statements.insert(statements.end(), tmp_statements.begin(),
                        tmp_statements.end());
    }
  }

  return statements;
}

std::vector<CCSTStatement *> dealloc_container(Variable *v,
                                               const std::string &cspace) {
  std::vector<CCSTStatement *> statements;

  if (v->container()) {
    std::vector<CCSTAssignExpr *> cap_remove_args;
    cap_remove_args.push_back(new CCSTPrimaryExprId(cspace));

    ProjectionType *container =
        dynamic_cast<ProjectionType *>(v->container()->type());
    Assert(container != 0x0,
           "Error: variables's container does not have type projection\n");

    ProjectionField *my_ref_field = container->get_field("my_ref");
    Assert(my_ref_field != 0x0,
           "Error: could not find my_ref field in projection\n");

    cap_remove_args.push_back(
        access(my_ref_field)); // access container my ref field

    statements.push_back(new CCSTExprStatement(
        function_call("glue_cap_remove", cap_remove_args)));
  }
  return statements;
}

std::vector<CCSTStatement *>
dealloc_containers_callee(Variable *v, const std::string &cspace,
                          LexicalScope *ls) {
  std::vector<CCSTStatement *> statements;
  if (v->dealloc_caller() && !v->dealloc_callee()) {
    statements = dealloc_container(v, cspace);
  }

  if (v->dealloc_callee()) {
    if (v->container() != 0x0) {
      std::vector<CCSTAssignExpr *> cap_remove_args;
      cap_remove_args.push_back(new CCSTPrimaryExprId(cspace));

      ProjectionType *container =
          dynamic_cast<ProjectionType *>(v->container()->type());
      Assert(container != 0x0,
             "Error: variables's container does not have type projection\n");

      ProjectionField *my_ref_field = container->get_field("my_ref");
      Assert(my_ref_field != 0x0,
             "Error: could not find my_ref field in projection\n");

      cap_remove_args.push_back(
          access(my_ref_field)); // access container my ref field

      statements.push_back(
          new CCSTExprStatement(function_call("cap_remove", cap_remove_args)));

      // remove all channels we received
      if (v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        ProjectionConstructorType *pct =
            dynamic_cast<ProjectionConstructorType *>(v->type());
        Assert(pct != 0x0,
               "Error: dynamic cast to projection constructor type failed\n");

        std::vector<std::pair<Variable *, Variable *>> other_chans =
            pct->channel_params_;
        for (auto p : other_chans) {
          std::vector<CCSTAssignExpr *> lcd_cap_delete_args;
          lcd_cap_delete_args.push_back(access(p.first));
          statements.push_back(new CCSTExprStatement(
              function_call("lcd_cap_delete", lcd_cap_delete_args)));
        }
      }
    }
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *tmp = dynamic_cast<ProjectionType *>(v->type());
    Assert(tmp != 0x0, "Error: dynamic cast to projection type failed\n");

    std::vector<ProjectionField *> chans = tmp->channels_;
    for (auto c : chans) {
      if (c->dealloc_callee()) {
        std::vector<CCSTAssignExpr *> lcd_cap_delete_args;
        lcd_cap_delete_args.push_back(access(c));
        statements.push_back(new CCSTExprStatement(
            function_call("lcd_cap_delete", lcd_cap_delete_args)));
      }
    }

    for (auto pf : *tmp) {
      if (pf->type()->num() == FUNCTION_TYPE && v->container() != 0x0 &&
          v->dealloc_callee()) {
        // set our hidden args struct equal to thing
        ProjectionType *v_container_type =
            dynamic_cast<ProjectionType *>(v->container()->type());
        Assert(v_container_type != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *v_container_real_field =
            v_container_type->get_field(v->type()->name());
        Assert(v_container_real_field != 0x0,
               "Error: could not find field in structure\n");

        ProjectionType *real_field_type =
            dynamic_cast<ProjectionType *>(v_container_real_field->type());
        Assert(real_field_type != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *pf_in_container =
            real_field_type->get_field(pf->identifier());
        Assert(pf_in_container != 0x0,
               "Error: could not find field in structure\n");

        std::vector<CCSTAssignExpr *> args;
        args.push_back(access(pf_in_container));
        const std::string &hidden_args_id =
            hidden_args_name(append_strings("_", construct_list_vars(pf)));
        statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
            new CCSTPrimaryExprId(hidden_args_id), equals(),
            function_call("LCD_TRAMPOLINE_TO_HIDDEN_ARGS", args))));

        // delete its t_handle field
        int err;
        Type *hidden_args_type = ls->lookup("trampoline_hidden_args", &err);
        Assert(hidden_args_type != 0x0,
               "Error: could not find a hidden args type in scope\n");
        ProjectionType *hidden_args_structure =
            dynamic_cast<ProjectionType *>(hidden_args_type);
        Assert(hidden_args_structure != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *t_handle_field =
            hidden_args_structure->get_field("t_handle");
        Assert(t_handle_field != 0x0,
               "Error: could not find t_handle field in projection\n");

        std::vector<CCSTAssignExpr *> kfree_args1;
        Parameter *tmp_hidden_args =
            new Parameter(hidden_args_structure, hidden_args_id, 1);

        Variable *accessor_save = t_handle_field->accessor();
        t_handle_field->set_accessor(tmp_hidden_args);

        kfree_args1.push_back(access(t_handle_field));
        statements.push_back(
            new CCSTExprStatement(function_call("kfree", kfree_args1)));

        t_handle_field->set_accessor(accessor_save);

        // delete it
        std::vector<CCSTAssignExpr *> kfree_args2;
        kfree_args2.push_back(access(tmp_hidden_args));
        statements.push_back(
            new CCSTExprStatement(function_call("kfree", kfree_args2)));
      }

      std::vector<CCSTStatement *> tmp_statements =
          dealloc_containers_callee(pf, cspace, ls);
      statements.insert(statements.end(), tmp_statements.begin(),
                        tmp_statements.end());
    }
  }

  if (v->dealloc_callee() && v->container() != 0x0) {
    std::vector<CCSTAssignExpr *> kfree_args;
    kfree_args.push_back(access(v->container()));
    statements.push_back(
        new CCSTExprStatement(function_call("kfree", kfree_args)));
  }

  return statements;
}

std::vector<CCSTStatement *>
dealloc_containers_caller(Variable *v, const std::string &cspace,
                          LexicalScope *ls) {
  std::vector<CCSTStatement *> statements;
  if (v->dealloc_callee() && !v->dealloc_caller()) {
    statements = dealloc_container(v, cspace);
  }

  if (v->dealloc_caller()) { // this is the problem. weirdddd
    if (v->container() != 0x0) {
      std::vector<CCSTAssignExpr *> cap_remove_args;
      cap_remove_args.push_back(new CCSTPrimaryExprId(cspace));

      ProjectionType *container =
          dynamic_cast<ProjectionType *>(v->container()->type());
      Assert(container != 0x0,
             "Error: variables's container does not have type projection\n");

      ProjectionField *my_ref_field = container->get_field("my_ref");
      Assert(my_ref_field != 0x0,
             "Error: could not find my_ref field in projection\n");

      cap_remove_args.push_back(
          access(my_ref_field)); // access container my ref field

      statements.push_back(new CCSTExprStatement(
          function_call("glue_cap_remove", cap_remove_args)));

      // remove all channels we recieved
      if (v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        ProjectionConstructorType *pct =
            dynamic_cast<ProjectionConstructorType *>(v->type());
        Assert(pct != 0x0,
               "Error: dynamic cast to projection constructor type failed\n");

        std::vector<std::pair<Variable *, Variable *>> other_chans =
            pct->channel_params_;
        for (auto p : other_chans) {
          std::vector<CCSTAssignExpr *> lcd_cap_delete_args;
          lcd_cap_delete_args.push_back(access(p.first));
          statements.push_back(new CCSTExprStatement(
              function_call("lcd_cap_delete", lcd_cap_delete_args)));
        }
      }
    }
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *tmp = dynamic_cast<ProjectionType *>(v->type());
    Assert(tmp != 0x0, "Error: dynamic cast to projection type failed\n");

    // remove channels we allocated
    std::vector<ProjectionField *> chans = tmp->channels_;
    for (auto c : chans) {
      if (c->dealloc_caller()) {
        std::vector<CCSTAssignExpr *> lcd_cap_delete_args;
        lcd_cap_delete_args.push_back(access(c));
        statements.push_back(new CCSTExprStatement(
            function_call("lcd_cap_delete", lcd_cap_delete_args)));
      }
    }

    for (auto pf : *tmp) {

      if (pf->type()->num() == FUNCTION_TYPE && v->container() != 0x0 &&
          v->dealloc_caller()) {
        // set our hidden args struct equal to thing
        ProjectionType *v_container_type =
            dynamic_cast<ProjectionType *>(v->container()->type());
        Assert(v_container_type != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *v_container_real_field =
            v_container_type->get_field(v->type()->name());
        Assert(v_container_real_field != 0x0,
               "Error: could not find field in structure\n");

        ProjectionType *real_field_type =
            dynamic_cast<ProjectionType *>(v_container_real_field->type());
        Assert(real_field_type != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *pf_in_container =
            real_field_type->get_field(pf->identifier());
        Assert(pf_in_container != 0x0,
               "Error: could not find field in structure\n");

        std::vector<CCSTAssignExpr *> args;
        args.push_back(access(pf_in_container));
        const std::string &hidden_args_id =
            hidden_args_name(append_strings("_", construct_list_vars(pf)));
        statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
            new CCSTPrimaryExprId(hidden_args_id), equals(),
            function_call("LCD_TRAMPOLINE_TO_HIDDEN_ARGS", args))));

        // delete its t_handle field
        int err;
        Type *hidden_args_type = ls->lookup("trampoline_hidden_args", &err);
        Assert(hidden_args_type != 0x0,
               "Error: could not find a hidden args type in scope\n");
        ProjectionType *hidden_args_structure =
            dynamic_cast<ProjectionType *>(hidden_args_type);
        Assert(hidden_args_structure != 0x0,
               "Error: dynamic cast to projection type failed\n");

        ProjectionField *t_handle_field =
            hidden_args_structure->get_field("t_handle");
        Assert(t_handle_field != 0x0,
               "Error: could not find t_handle field in projection\n");

        std::vector<CCSTAssignExpr *> kfree_args1;
        Parameter *tmp_hidden_args =
            new Parameter(hidden_args_structure, hidden_args_id, 1);

        Variable *accessor_save = t_handle_field->accessor();
        t_handle_field->set_accessor(tmp_hidden_args);

        kfree_args1.push_back(access(t_handle_field));
        statements.push_back(
            new CCSTExprStatement(function_call("kfree", kfree_args1)));

        t_handle_field->set_accessor(accessor_save);

        // delete it
        std::vector<CCSTAssignExpr *> kfree_args2;
        kfree_args2.push_back(access(tmp_hidden_args));
        statements.push_back(
            new CCSTExprStatement(function_call("kfree", kfree_args2)));
      } else {
        std::vector<CCSTStatement *> tmp_statements =
            dealloc_containers_caller(pf, cspace, ls);
        statements.insert(statements.end(), tmp_statements.begin(),
                          tmp_statements.end());
      }
    }
  }

  if (v->dealloc_caller() && v->container() != 0x0) {
    std::vector<CCSTAssignExpr *> kfree_args;
    kfree_args.push_back(access(v->container()));
    statements.push_back(
        new CCSTExprStatement(function_call("kfree", kfree_args)));
  }

  return statements;
}
