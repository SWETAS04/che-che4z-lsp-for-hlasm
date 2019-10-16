#include <stdexcept>
#include <limits>

#include "symbol_attributes.h"
#include "../../ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

const symbol_attributes::type_attr symbol_attributes::undef_type = ebcdic_encoding::to_ebcdic('U');

const symbol_attributes::len_attr symbol_attributes::undef_length = static_cast<len_attr>(-1);

const symbol_attributes::scale_attr symbol_attributes::undef_scale = std::numeric_limits<scale_attr>::max();

symbol_attributes symbol_attributes::make_section_attrs() 
{ return symbol_attributes(symbol_origin::SECT, ebcdic_encoding::to_ebcdic('J'), 1); }
symbol_attributes symbol_attributes::make_machine_attrs(symbol_attributes::len_attr length) 
{ return symbol_attributes(symbol_origin::MACH, ebcdic_encoding::to_ebcdic('I'), length); }

data_attr_kind symbol_attributes::transform_attr(char c)
{
	c = (char)std::toupper(c);
	switch (c)
	{
	case 'D':
		return data_attr_kind::D;
	case 'O':
		return data_attr_kind::O;
	case 'N':
		return data_attr_kind::N;
	case 'S':
		return data_attr_kind::S;
	case 'K':
		return data_attr_kind::K;
	case 'I':
		return data_attr_kind::I;
	case 'L':
		return data_attr_kind::L;
	case 'T':
		return data_attr_kind::T;
	default:
		return data_attr_kind::UNKNOWN;
	}
}

bool symbol_attributes::needs_ordinary(data_attr_kind attribute)
{
	return attribute == data_attr_kind::D || attribute == data_attr_kind::L || attribute == data_attr_kind::O
		|| attribute == data_attr_kind::S || attribute == data_attr_kind::I;
}

bool symbol_attributes::ordinary_allowed(data_attr_kind attribute)
{
	return attribute == data_attr_kind::L || attribute == data_attr_kind::I || attribute == data_attr_kind::S || attribute == data_attr_kind::T;
}

symbol_attributes::value_t symbol_attributes::default_value(data_attr_kind attribute)
{
	switch (attribute)
	{
	case data_attr_kind::T:
		return undef_type;
	case data_attr_kind::L:
		return 1;
	default:
		return 0;
	}
}

symbol_attributes::symbol_attributes(symbol_origin origin)
	: origin(origin), type_(undef_type), length_(undef_length), scale_(undef_scale), integer_(undef_length) {}

symbol_attributes::symbol_attributes(symbol_origin origin, type_attr type, len_attr length, scale_attr scale, len_attr integer)
	: origin(origin), type_(type), length_(length), scale_(scale),integer_(integer) {}

symbol_attributes::type_attr symbol_attributes::type() const { return type_; }

symbol_attributes::len_attr symbol_attributes::length() const { return length_; }

void symbol_attributes::length(len_attr value)
{
	length_ == undef_length ? length_ = value : throw std::runtime_error("value can be assigned only once");
}

void symbol_attributes::scale(scale_attr value)
{
	scale_ == undef_scale ? scale_ = value : throw std::runtime_error("value can be assigned only once");
}

symbol_attributes::scale_attr symbol_attributes::scale() const { return scale_; }


symbol_attributes::len_attr symbol_attributes::integer() const { return integer_; }

bool symbol_attributes::is_defined(data_attr_kind attribute) const
{
	switch (attribute)
	{
	case data_attr_kind::T:
		return true;
	case data_attr_kind::L:
		return length_ != undef_length;
	case data_attr_kind::S:
		return scale_ != undef_scale;
	case data_attr_kind::I:
		return scale_ != undef_scale && length_ != undef_length;
	default:
		return false;
	}
}

bool symbol_attributes::can_have_SI_attr() const
{
	return origin == symbol_origin::DAT &&
		(type_ == ebcdic_encoding::a2e['D'] || type_ == ebcdic_encoding::a2e['E'] || type_ == ebcdic_encoding::a2e['F'] 
			|| type_ == ebcdic_encoding::a2e['G'] || type_ == ebcdic_encoding::a2e['H'] || type_ == ebcdic_encoding::a2e['K'] 
			|| type_ == ebcdic_encoding::a2e['L'] || type_ == ebcdic_encoding::a2e['P'] || type_ == ebcdic_encoding::a2e['Z']);
}

symbol_attributes::value_t symbol_attributes::get_attribute_value(data_attr_kind attribute) const
{
	switch (attribute)
	{
	case data_attr_kind::T:
		return type_;
	case data_attr_kind::L:
		return length_ == undef_length ? 1 : length_;
	case data_attr_kind::S:
		return scale_ == undef_scale ? 0 : scale_;
	case data_attr_kind::I:
		return integer();
	default:
		return 0;
	}
}
