/* $Id$ */

%{

typedef struct {
	union {
		double	number;
		struct {
			double x;
			double y;
			double z;
		}	vector;
		char	*string;
	} v;
	int lineno;
} YYSTYPE;

%}

%token DIM
%token MAG SIZE SPACE SPANS OFFSET CONTAINS
%token <v.string> STRING
%type <v.vector> vector

%%

grammar		: /* empty */
		| grammar '\n'
		| grammar conf '\n'
		| grammar error '\n'
		;

optnl		: '\n' optnl
		|
		;

conf		: DIM string optnl '{' optnl {
			free($2);
		}
		opts '}' {
		}
		;

opts		:
		;

%%

