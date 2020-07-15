#include "marshaling.h"

void idlc::process_marshal_units(gsl::span<const marshal_unit> units)
{
	for (const auto& unit : units) {
		log_debug("RPC marshal unit ", unit.identifier);
		for (const auto& arg : unit.sig->arguments()) {
			switch (arg->kind()) {
			case idlc::field_kind::var: {
				const auto& var_arg = arg->get<idlc::field_kind::var>();
				const auto& arg_type = var_arg.get_type();
				if (!arg_type.stars()) {
					log_debug("\tArgument ", var_arg.identifier(), " is of value type and will be copy-marshaled");
				}

				break;
			}

			case idlc::field_kind::rpc:
				break;
			}
		}

		const auto& rf = unit.sig->return_field();
		switch (rf.kind()) {
		case idlc::field_kind::var: {
			const auto& var_arg = rf.get<idlc::field_kind::var>();
			const auto& arg_type = var_arg.get_type();
			if (!arg_type.stars()) {
				log_debug("\tReturn value is of value type and will be copy-marshaled");
			}

			break;
		}

		case idlc::field_kind::rpc:
			break;
		}
	}
}