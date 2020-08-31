#pragma once

#include "json5_builder.hpp"

#include <charconv>
#include <fstream>
#include <sstream>

namespace json5 {

// Parse json5::document from stream
error from_stream( std::istream &is, document &doc );

// Parse json5::document from string
error from_string( const std::string &str, document &doc );

// Parse json5::document from file
error from_file( const std::string &fileName, document &doc );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class parser final : builder
{
public:
	parser( document &doc, detail::char_source &chars ) : builder( doc ), _chars( chars ) { }

	error parse();

private:
	char next() { return _chars.next(); }
	char peek() { return _chars.peek(); }
	bool eof() const { return _chars.eof(); }
	error make_error( int type ) const noexcept { return _chars.make_error( type ); }

	enum class token_type
	{
		unknown, identifier, string, number, colon, comma,
		object_begin, object_end, array_begin, array_end,
		literal_true, literal_false, literal_null
	};

	error parse_value( value &result );
	error parse_object();
	error parse_array();
	error peek_next_token( token_type &result );
	error parse_number( double &result );
	error parse_string( detail::string_offset &result );
	error parse_identifier( detail::string_offset &result );
	error parse_literal( token_type &result );

	detail::char_source &_chars;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

class stl_istream : public char_source
{
public:
	stl_istream( std::istream &is ) : _is( is ) { }

	char next() override
	{
		if ( _is.peek() == '\n' )
		{
			_column = 0;
			++_line;
		}

		++_column;
		return _is.get();
	}

	char peek() override { return _is.peek(); }

	bool eof() const override { return _is.eof(); }

protected:
	std::istream &_is;
};

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse()
{
	reset();

	if ( auto err = parse_value( _doc ) )
		return err;

	if ( !_doc.is_array() && !_doc.is_object() )
		return make_error( error::invalid_root );

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_value( value &result )
{
	token_type tt = token_type::unknown;
	if ( auto err = peek_next_token( tt ) )
		return err;

	switch ( tt )
	{
		case token_type::number:
		{
			if ( double number = 0.0; auto err = parse_number( number ) )
				return err;
			else
				result = value( number );
		}
		break;

		case token_type::string:
		{
			if ( detail::string_offset offset = 0; auto err = parse_string( offset ) )
				return err;
			else
				result = new_string( offset );
		}
		break;

		case token_type::identifier:
		{
			if ( token_type lit = token_type::unknown; auto err = parse_literal( lit ) )
				return err;
			else
			{
				if ( lit == token_type::literal_true )
					result = value( true );
				else if ( lit == token_type::literal_false )
					result = value( false );
				else if ( lit == token_type::literal_null )
					result = value();
				else
					return make_error( error::invalid_literal );
			}
		}
		break;

		case token_type::object_begin:
		{
			push_object();
			{
				if ( auto err = parse_object() )
					return err;
			}
			result = pop();
		}
		break;

		case token_type::array_begin:
		{
			push_array();
			{
				if ( auto err = parse_array() )
					return err;
			}
			result = pop();
		}
		break;

		default:
			return make_error( error::syntax_error );
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_object()
{
	next(); // Consume '{'

	bool expectComma = false;
	while ( !eof() )
	{
		token_type tt = token_type::unknown;
		if ( auto err = peek_next_token( tt ) )
			return err;

		detail::string_offset keyOffset;

		switch ( tt )
		{
			case token_type::identifier:
			case token_type::string:
			{
				if ( expectComma )
					return make_error( error::comma_expected );

				if ( auto err = parse_identifier( keyOffset ) )
					return err;
			}
			break;

			case token_type::object_end:
				next(); // Consume '}'
				return { error::none };

			case token_type::comma:
				if ( !expectComma )
					return make_error( error::syntax_error );

				next(); // Consume ','
				expectComma = false;
				continue;

			default:
				return expectComma ? make_error( error::comma_expected ) : make_error( error::syntax_error );
		}

		if ( auto err = peek_next_token( tt ) )
			return err;

		if ( tt != token_type::colon )
			return make_error( error::colon_expected );

		next(); // Consume ':'

		value newValue;
		if ( auto err = parse_value( newValue ) )
			return err;

		( *this )[keyOffset] = newValue;
		expectComma = true;
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_array()
{
	next(); // Consume '['

	bool expectComma = false;
	while ( !eof() )
	{
		token_type tt = token_type::unknown;
		if ( auto err = peek_next_token( tt ) )
			return err;

		if ( tt == token_type::array_end && next() ) // Consume ']'
			return { error::none };
		else if ( expectComma )
		{
			expectComma = false;

			if ( tt != token_type::comma )
				return make_error( error::comma_expected );

			next(); // Consume ','
			continue;
		}

		value newValue;
		if ( auto err = parse_value( newValue ) )
			return err;

		( *this ) += newValue;
		expectComma = true;
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::peek_next_token( token_type &result )
{
	enum class comment_type { none, line, block } parsingComment = comment_type::none;

	while ( !eof() )
	{
		char ch = peek();
		if ( ch == '\n' )
		{
			if ( parsingComment == comment_type::line )
				parsingComment = comment_type::none;
		}
		else if ( parsingComment != comment_type::none || ch <= 32 )
		{
			if ( parsingComment == comment_type::block && ch == '*' && next() ) // Consume '*'
			{
				if ( peek() == '/' )
					parsingComment = comment_type::none;
			}
		}
		else if ( ch == '/' && next() ) // Consume '/'
		{
			if ( peek() == '/' )
				parsingComment = comment_type::line;
			else if ( peek() == '*' )
				parsingComment = comment_type::block;
			else
				return make_error( error::syntax_error );
		}
		else if ( strchr( "{}[]:,", ch ) )
		{
			if ( ch == '{' )
				result = token_type::object_begin;
			else if ( ch == '}' )
				result = token_type::object_end;
			else if ( ch == '[' )
				result = token_type::array_begin;
			else if ( ch == ']' )
				result = token_type::array_end;
			else if ( ch == ':' )
				result = token_type::colon;
			else if ( ch == ',' )
				result = token_type::comma;

			return { error::none };
		}
		else if ( isalpha( ch ) || ch == '_' )
		{
			result = token_type::identifier;
			return { error::none };
		}
		else if ( isdigit( ch ) || ch == '.' || ch == '+' || ch == '-' )
		{
			if ( ch == '+' ) next(); // Consume leading '+'

			result = token_type::number;
			return { error::none };
		}
		else if ( ch == '"' || ch == '\'' )
		{
			result = token_type::string;
			return { error::none };
		}
		else
			return make_error( error::syntax_error );

		next();
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_number( double &result )
{
	char buff[256] = { };
	size_t length = 0;

	while ( !eof() && length < sizeof( buff ) )
	{
		buff[length++] = next();

		char ch = peek();
		if ( ch <= 32 || ch == ',' || ch == '}' || ch == ']' )
			break;
	}

	auto convResult = std::from_chars( buff, buff + length, result );

	if ( convResult.ec != std::errc() )
		return make_error( error::syntax_error );

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_string( detail::string_offset &result )
{
	static constexpr char *hexChars = "0123456789abcdefABCDEF";

	bool singleQuoted = peek() == '\'';
	next(); // Consume '\'' or '"'

	result = string_buffer_offset();

	while ( !eof() )
	{
		char ch = peek();
		if ( ( ( singleQuoted && ch == '\'' ) || ( !singleQuoted && ch == '"' ) ) && next() ) // Consume '\'' or '"'
			break;
		else if ( ch == '\\' && next() ) // Consume '\\'
		{
			ch = peek();
			if ( ch == '\n' || ch == 'v' || ch == 'f' )
				next();
			else if ( ch == 't' && next() )
				string_buffer_add( '\t' );
			else if ( ch == 'n' && next() )
				string_buffer_add( '\n' );
			else if ( ch == 'r' && next() )
				string_buffer_add( '\r' );
			else if ( ch == 'b' && next() )
				string_buffer_add( '\b' );
			else if ( ch == '\\' && next() )
				string_buffer_add( '\\' );
			else if ( ch == '\'' && next() )
				string_buffer_add( '\'' );
			else if ( ch == '"' && next() )
				string_buffer_add( '"' );
			else if ( ch == '\\' && next() )
				string_buffer_add( '\\' );
			else if ( ch == '/' && next() )
				string_buffer_add( '/' );
			else if ( ch == '0' && next() )
				string_buffer_add( 0 );
			else if ( ( ch == 'x' || ch == 'u' ) && next() )
			{
				char code[5] = { };

				for ( size_t i = 0, S = ( ch == 'x' ) ? 2 : 4; i < S; ++i )
					if ( !strchr( hexChars, code[i] = next() ) )
						return make_error( error::invalid_escape_seq );

				uint64_t unicodeChar = 0;
				std::from_chars( code, code + 5, unicodeChar, 16 );
				string_buffer_add_utf8( uint32_t( unicodeChar ) );
			}
			else
				return make_error( error::invalid_escape_seq );
		}
		else
			string_buffer_add( next() );
	}

	if ( eof() )
		return make_error( error::unexpected_end );

	string_buffer_add( 0 );
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_identifier( detail::string_offset &result )
{
	result = string_buffer_offset();

	char firstCh = peek();
	bool isString = ( firstCh == '\'' ) || ( firstCh == '"' );

	if ( isString && next() ) // Consume '\'' or '"'
	{
		char ch = peek();
		if ( !isalpha( ch ) && ch != '_' )
			return make_error( error::syntax_error );
	}

	while ( !eof() )
	{
		string_buffer_add( next() );

		char ch = peek();
		if ( !isalpha( ch ) && !isdigit( ch ) && ch != '_' )
			break;
	}

	if ( isString && firstCh != next() ) // Consume '\'' or '"'
		return make_error( error::syntax_error );

	string_buffer_add( 0 );
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error parser::parse_literal( token_type &result )
{
	char ch = peek();

	// "true"
	if ( ch == 't' )
	{
		if ( next() && next() == 'r' && next() == 'u' && next() == 'e' )
		{
			result = token_type::literal_true;
			return { error::none };
		}
	}
	// "false"
	else if ( ch == 'f' )
	{
		if ( next() && next() == 'a' && next() == 'l' && next() == 's' && next() == 'e' )
		{
			result = token_type::literal_false;
			return { error::none };
		}
	}
	// "null"
	else if ( ch == 'n' )
	{
		if ( next() && next() == 'u' && next() == 'l' && next() == 'l' )
		{
			result = token_type::literal_null;
			return { error::none };
		}
	}

	return make_error( error::invalid_literal );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline error from_stream( std::istream &is, document &doc )
{
	parser r( doc, detail::stl_istream( is ) );
	return r.parse();
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_string( const std::string &str, document &doc )
{
	std::istringstream is( str );
	return from_stream( is, doc );
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_file( const std::string &fileName, document &doc )
{
	std::ifstream ifs( fileName );
	return from_stream( ifs, doc );
}

} // namespace json5
