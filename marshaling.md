Take the following example IDL:

	projection <struct bar> bar_proj {}

	projection <struct foo> foo_proj {
		int [in] a; // the annotation here is only meaningful if the projection is passed by pointer!
		int [out] b;
		int [in, out] c;
		projection bar_proj [in, out] *d; // If we allow "in, out" inference, this is ambiguous
		// Are we applying the annotations to the field of bar_proj
		// Or are we applying it to the "d" field of foo_proj?
	}

	rpc int do_widget(int flags, projection foo_proj [bind(callee)] *widget);

("marshal" describes a literal append of the field value to rpc message buffer)

Marshal buffer has marshaled fields in left-to-right order, and if the return value is present, it comes last

Actions for caller:

	0	parameter		// var_0 = flags
	1	parameter		// var_1 = widget
		marshal 0		// buffer[++slot] = var_0
		marshal 1		// buffer[++slot] = var_1
	2	get 1 0			// var_2 = var_1->a
	3	get 1 2			// var_3 = var_1->c
	4	get 1 3			// var_4 = var_1->d
		marshal 2		// buffer[++slot] = var_2
		marshal 3		// buffer[++slot] = var_3
		marshal 4		// buffer[++slot] = var_4
		send_rpc		// send(rpc, buffer); slot = 0;
	5	unmarshal		// var_5 = buffer[++slot]
	6	unmarshal		// var_6 = buffer[++slot]
	7	unmarshal		// var_7 = buffer[++slot]
	8	unmarshal		// var_8 = buffer[++slot]
		set 1 1 5		// var_1->b = var_5
		set 1 2 6		// var_1->c = var_6
		set 1 3 7		// var_1->d = var_7
		return 8		// return var_8

Actions for callee:

	0	unmarshal		// var_0 = buffer[++slot]
	1	unmarshal		// var_1 = buffer[++slot]
	2	get_cspace 1	// var_2 = get_cspace(var_1)
	3	unmarshal		// var_3 = buffer[++slot]
	4	unmarshal		// var_4 = buffer[++slot]
	5	unmarshal		// var_5 = buffer[++slot]
		set 1 0 3		// var_1->a = var_3
		set 1 2 4		// var_1->c = var_4
		set 1 3 5		// var_1->d = var_5
		argument 0		// flags = var_0
		argument 1		// widget = var_1
		call_impl		// var_ret_val = <impl>(flags, widget); slot = 0;
	6	return_val		// var_6 = var_ret_val
	7	get 1 1			// var_7 = var_1->b
	8	get 1 2			// var_8 = var_1->c
	9	get 1 3			// var_9 = var_1->d
		marshal 7		// buffer[++slot] = var_7
		marshal 8		// buffer[++slot] = var_8
		marshal 9		// buffer[++slot] = var_9
		marshal 6		// buffer[++slot] = var_6
		reply			// reply(buffer)

by-value projection as undefined behavior? or pass all fields?