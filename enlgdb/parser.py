"""Parser for enlgdb (Natural English SQL & Database Language)."""

from typing import List, Optional, Any, Dict
from enlgdb.tokens import Token, TokenType
from enlgdb.ast_nodes import (
    ProgramNode, DomainHeaderNode, CreateTableNode, ColumnDefNode,
    CreateDatabaseNode, UseDatabaseNode, ShowDatabasesNode, ShowTablesNode, DropDatabaseNode,
    InsertNode, SelectNode, OrderByNode, JoinNode,
    UpdateNode, DeleteNode, DropTableNode, TruncateTableNode,
    AlterTableNode, BinaryOpNode, UnaryOpNode, FunctionCallNode,
    IdentifierNode, LiteralNode, ASTNode, HintNode
)


class ParserError(Exception):
    def __init__(self, message: str, token: Optional[Token] = None, hint: str = ""):
        self.message = message
        self.token = token
        self.hint = hint
        loc = f" at L{token.line}:C{token.column}" if token else ""
        hint_text = f"\n  💡 Hint: {hint}" if hint else ""
        super().__init__(f"Parser error{loc}: {message}{hint_text}")


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0

    def current_token(self) -> Token:
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return self.tokens[-1]

    def peek_token(self, offset: int = 1) -> Token:
        idx = self.pos + offset
        if idx < len(self.tokens):
            return self.tokens[idx]
        return self.tokens[-1]

    def match(self, *expected_types: TokenType) -> bool:
        return self.current_token().type in expected_types

    def consume(self, expected_type: TokenType, error_msg: str = "", hint: str = "") -> Token:
        tok = self.current_token()
        if tok.type == expected_type:
            self.pos += 1
            return tok
        msg = error_msg or f"Expected token {expected_type.name}, but found {tok.type.name} ('{tok.value}')"
        raise ParserError(msg, tok, hint)

    def skip_newlines(self):
        while self.match(TokenType.NEWLINE):
            self.pos += 1

    def skip_silent_words(self):
        """Skips conversational filler words like 'the', 'a', 'an', 'please', 'records', 'rows', etc."""
        silent_types = {
            TokenType.THE, TokenType.A, TokenType.AN,
            TokenType.PLEASE, TokenType.KINDLY, TokenType.SIMPLY, TokenType.JUST,
            TokenType.THAT, TokenType.WHICH,
            TokenType.RECORD, TokenType.RECORDS,
            TokenType.ROW, TokenType.ROWS,
            TokenType.ENTRY, TokenType.ENTRIES,
            TokenType.DATA, TokenType.ITEM, TokenType.ITEMS
        }
        while self.match(*silent_types):
            self.pos += 1

    def parse_hints_dict(self) -> Dict[str, Any]:
        """Parses 'hint key: val, key2: val2' into a dictionary."""
        self.consume(TokenType.HINT)
        hints: Dict[str, Any] = {}
        while True:
            self.skip_silent_words()
            key_tok = self.current_token()
            if self.match(TokenType.IDENTIFIER, TokenType.INDEX, TokenType.DEFAULT):
                self.pos += 1
                key = str(key_tok.value)
            else:
                break
            if self.match(TokenType.COLON, TokenType.EQUALS):
                self.pos += 1
            val_expr = self.parse_expression()
            if isinstance(val_expr, LiteralNode):
                hints[key] = val_expr.value
            elif isinstance(val_expr, IdentifierNode):
                hints[key] = val_expr.name
            else:
                hints[key] = val_expr
            
            if self.match(TokenType.COMMA):
                self.consume(TokenType.COMMA)
            else:
                break
        return hints

    def parse(self) -> ProgramNode:
        self.skip_newlines()
        header = None

        # Check for type enlgdb header
        if self.match(TokenType.TYPE):
            self.consume(TokenType.TYPE)
            tok = self.consume(TokenType.ENLGDB, "Expected 'enlgdb' after 'type'", "Declare 'type enlgdb' at the top of the file.")
            header = DomainHeaderNode(domain="enlgdb")
            self.skip_newlines()

        statements: List[ASTNode] = []
        while not self.match(TokenType.EOF):
            self.skip_newlines()
            if self.match(TokenType.EOF):
                break

            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
            self.skip_newlines()

        return ProgramNode(header=header, statements=statements)

    def parse_statement(self) -> ASTNode:
        self.skip_silent_words()
        tok = self.current_token()

        # HINT ...
        if self.match(TokenType.HINT):
            return self.parse_standalone_hint()

        # CREATE TABLE / DATABASE ...
        elif self.match(TokenType.CREATE):
            if self.peek_token(1).type == TokenType.DATABASE:
                return self.parse_create_database()
            return self.parse_create_table()

        # USE DATABASE ...
        elif self.match(TokenType.USE):
            return self.parse_use_database()

        # SHOW DATABASES / TABLES ... OR SHOW QUERY
        elif self.match(TokenType.SHOW):
            if self.peek_token(1).type in (TokenType.DATABASES, TokenType.DATABASE, TokenType.TABLES, TokenType.TABLE):
                return self.parse_show()
            return self.parse_select()

        # INSERT / SAVE / PUT INTO ...
        elif self.match(TokenType.INSERT, TokenType.SAVE, TokenType.PUT):
            return self.parse_insert()

        # SELECT / FIND / FETCH / GET ...
        elif self.match(TokenType.SELECT, TokenType.FIND, TokenType.FETCH, TokenType.GET):
            return self.parse_select()

        # COUNT ... (e.g. count records in "accounts" where ...)
        elif self.match(TokenType.COUNT):
            return self.parse_count_query()

        # UPDATE / CHANGE ...
        elif self.match(TokenType.UPDATE, TokenType.CHANGE):
            return self.parse_update()

        # IN <table_name> UPDATE / CHANGE / SET ...
        elif self.match(TokenType.IN):
            return self.parse_in_statement()

        # DELETE / REMOVE ...
        elif self.match(TokenType.DELETE, TokenType.REMOVE):
            return self.parse_delete()

        # DROP TABLE / DATABASE / COLUMN ...
        elif self.match(TokenType.DROP):
            if self.peek_token(1).type == TokenType.DATABASE:
                return self.parse_drop_database()
            return self.parse_drop_table()

        # TRUNCATE TABLE ...
        elif self.match(TokenType.TRUNCATE):
            return self.parse_truncate_table()

        # ALTER TABLE ...
        elif self.match(TokenType.ALTER):
            return self.parse_alter_table()

        else:
            raise ParserError(
                f"Unexpected database statement beginning with '{tok.value}'",
                tok,
                "Valid statements: create table/database, use database, show tables, insert/save into, select/find/fetch/get, update, delete/remove, count, hint."
            )

    # -------------------------------------------------------------
    # 0. DATABASE LIFECYCLE (CREATE, USE, SHOW, DROP)
    # -------------------------------------------------------------
    def parse_create_database(self) -> CreateDatabaseNode:
        self.consume(TokenType.CREATE)
        self.consume(TokenType.DATABASE)
        db_name = self.parse_table_or_column_name("database name")
        return CreateDatabaseNode(db_name=db_name)

    def parse_use_database(self) -> UseDatabaseNode:
        self.consume(TokenType.USE)
        if self.match(TokenType.DATABASE):
            self.consume(TokenType.DATABASE)
        db_name = self.parse_table_or_column_name("database name")
        return UseDatabaseNode(db_name=db_name)

    def parse_show(self) -> ASTNode:
        self.consume(TokenType.SHOW)
        if self.match(TokenType.DATABASES):
            self.consume(TokenType.DATABASES)
            return ShowDatabasesNode()
        elif self.match(TokenType.TABLES, TokenType.TABLE):
            self.pos += 1
            db_name = None
            if self.match(TokenType.FROM, TokenType.INTO):
                self.pos += 1
                db_name = self.parse_table_or_column_name("database name")
            return ShowTablesNode(db_name=db_name)
        else:
            raise ParserError("Expected 'databases' or 'tables' after 'show'", self.current_token(),
                              "Use: show databases OR show tables [from database_name]")

    def parse_drop_database(self) -> DropDatabaseNode:
        self.consume(TokenType.DROP)
        self.consume(TokenType.DATABASE)
        db_name = self.parse_table_or_column_name("database name")
        confirm_tok = self.parse_confirmation(f"drop database {db_name}", f"drop database {db_name} confirmed")
        return DropDatabaseNode(db_name=db_name, confirmation_token=confirm_tok)

    # -------------------------------------------------------------
    # 1. CREATE TABLE
    # -------------------------------------------------------------
    def parse_create_table(self) -> CreateTableNode:
        self.consume(TokenType.CREATE)
        self.consume(TokenType.TABLE, "Expected 'table' after 'create'", "Use: create table \"table_name\" with:")

        table_name = self.parse_table_or_column_name("table name")
        self.consume(TokenType.WITH, "Expected 'with:' after table name", "Use: create table \"table_name\" with:")
        if self.match(TokenType.COLON):
            self.consume(TokenType.COLON)

        self.skip_newlines()
        self.consume(TokenType.INDENT, "Expected indented column definitions block", "Indent column definitions by 4 spaces under 'with:'.")

        columns: List[ColumnDefNode] = []
        table_hints: Dict[str, Any] = {}
        while not self.match(TokenType.DEDENT, TokenType.EOF):
            self.skip_newlines()
            if self.match(TokenType.DEDENT, TokenType.EOF):
                break

            if self.match(TokenType.HINT):
                table_hints.update(self.parse_hints_dict())
            else:
                col_def = self.parse_column_def()
                columns.append(col_def)
            self.skip_newlines()

        self.consume(TokenType.DEDENT, "Expected dedent after column block")
        return CreateTableNode(table_name=table_name, columns=columns, hints=table_hints)

    def parse_column_def(self) -> ColumnDefNode:
        col_name = self.parse_table_or_column_name("column name")
        self.consume(TokenType.AS, "Expected 'as' after column name", "Use: column_name as <type> [constraints]")

        # Data type
        type_tok = self.current_token()
        type_map = {
            TokenType.TYPE_INTEGER: "INTEGER",
            TokenType.TYPE_TEXT: "TEXT",
            TokenType.TYPE_REAL: "REAL",
            TokenType.TYPE_BOOLEAN: "BOOLEAN",
            TokenType.TYPE_TIMESTAMP: "TIMESTAMP",
            TokenType.TYPE_BLOB: "BLOB",
            TokenType.TYPE_JSON: "JSON"
        }
        if type_tok.type not in type_map:
            raise ParserError(f"Unknown data type '{type_tok.value}'", type_tok,
                              "Valid types are: integer, text, real, boolean, timestamp, blob, json.")
        self.pos += 1
        data_type = type_map[type_tok.type]

        # Constraints
        is_primary_key = False
        autoincrement = False
        not_null = False
        unique = False
        default_val = None
        references_table = None
        references_col = None
        col_hints: Dict[str, Any] = {}

        while not self.match(TokenType.NEWLINE, TokenType.DEDENT, TokenType.EOF):
            if self.match(TokenType.PRIMARY):
                self.consume(TokenType.PRIMARY)
                self.consume(TokenType.KEY, "Expected 'key' after 'primary'")
                is_primary_key = True
            elif self.match(TokenType.AUTOINCREMENT):
                self.consume(TokenType.AUTOINCREMENT)
                autoincrement = True
            elif self.match(TokenType.NOT):
                self.consume(TokenType.NOT)
                self.consume(TokenType.NULL, "Expected 'null' after 'not'")
                not_null = True
            elif self.match(TokenType.UNIQUE):
                self.consume(TokenType.UNIQUE)
                unique = True
            elif self.match(TokenType.DEFAULT):
                self.consume(TokenType.DEFAULT)
                default_val = self.parse_literal_or_constant()
            elif self.match(TokenType.REFERENCES):
                self.consume(TokenType.REFERENCES)
                references_table = self.parse_table_or_column_name("referenced table")
                if self.match(TokenType.LPAREN):
                    self.consume(TokenType.LPAREN)
                    references_col = self.parse_table_or_column_name("referenced column")
                    self.consume(TokenType.RPAREN)
            elif self.match(TokenType.HINT):
                col_hints.update(self.parse_hints_dict())
            else:
                break

        return ColumnDefNode(
            name=col_name,
            data_type=data_type,
            is_primary_key=is_primary_key,
            autoincrement=autoincrement,
            not_null=not_null,
            unique=unique,
            default_value=default_val,
            references_table=references_table,
            references_column=references_col,
            hints=col_hints
        )

    # -------------------------------------------------------------
    # 2. INSERT INTO / SAVE / PUT
    # -------------------------------------------------------------
    def parse_insert(self) -> InsertNode:
        self.consume(self.current_token().type)  # INSERT, SAVE, or PUT
        self.skip_silent_words()
        if self.match(TokenType.INTO):
            self.consume(TokenType.INTO)
        self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")
        self.skip_silent_words()

        if self.match(TokenType.VALUES, TokenType.WITH):
            self.consume(self.current_token().type)
        if self.match(TokenType.COLON):
            self.consume(TokenType.COLON)

        self.skip_newlines()
        values: Dict[str, Any] = {}

        if self.match(TokenType.INDENT):
            self.consume(TokenType.INDENT)
            while not self.match(TokenType.DEDENT, TokenType.EOF):
                self.skip_newlines()
                if self.match(TokenType.DEDENT, TokenType.EOF):
                    break
                k, v = self.parse_key_value_assignment()
                values[k] = v
                self.skip_newlines()
            self.consume(TokenType.DEDENT)
        else:
            # Inline single line key-values
            k, v = self.parse_key_value_assignment()
            values[k] = v
            while self.match(TokenType.COMMA, TokenType.AND):
                self.consume(self.current_token().type)
                self.skip_silent_words()
                k, v = self.parse_key_value_assignment()
                values[k] = v

        hints: Dict[str, Any] = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return InsertNode(table_name=table_name, values=values, hints=hints)

    def parse_key_value_assignment(self) -> tuple:
        self.skip_silent_words()
        key = self.parse_table_or_column_name("field name")
        self.skip_silent_words()
        if self.match(TokenType.COLON, TokenType.EQUALS, TokenType.AS, TokenType.TO, TokenType.IS):
            self.consume(self.current_token().type)

        self.skip_silent_words()
        val = self.parse_expression()
        return key, val

    # -------------------------------------------------------------
    # 3. SELECT (DQL) / FIND / FETCH / GET / SHOW
    # -------------------------------------------------------------
    def parse_select(self) -> SelectNode:
        self.consume(self.current_token().type)  # SELECT, FIND, FETCH, GET, SHOW
        self.skip_silent_words()
        distinct = False
        if self.match(TokenType.DISTINCT):
            self.consume(TokenType.DISTINCT)
            distinct = True
            self.skip_silent_words()

        limit = None
        # Support: find top 10 ...
        if self.match(TokenType.TOP):
            self.consume(TokenType.TOP)
            self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer number after 'top'")
            limit = int(num_tok.value)
            self.skip_silent_words()

        fields: List[Any] = []
        if self.match(TokenType.ALL, TokenType.STAR):
            self.pos += 1
            fields.append("*")
            self.skip_silent_words()
        elif self.match(TokenType.FROM, TokenType.IN):
            # e.g. find from "users" or find top 5 from "users"
            fields.append("*")
        else:
            fields.append(self.parse_select_field())
            self.skip_silent_words()
            while self.match(TokenType.COMMA, TokenType.AND):
                self.consume(self.current_token().type)
                self.skip_silent_words()
                fields.append(self.parse_select_field())
                self.skip_silent_words()

        if self.match(TokenType.FROM, TokenType.IN):
            self.consume(self.current_token().type)
        else:
            raise ParserError("Expected 'from' or 'in' in select statement", self.current_token(), "Use: select ... from \"table_name\"")

        self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")
        self.skip_silent_words()

        # Joins
        joins: List[JoinNode] = []
        while self.match(TokenType.JOIN, TokenType.INNER, TokenType.LEFT, TokenType.RIGHT):
            join_type = "INNER"
            if self.match(TokenType.LEFT):
                self.consume(TokenType.LEFT)
                join_type = "LEFT"
            elif self.match(TokenType.RIGHT):
                self.consume(TokenType.RIGHT)
                join_type = "RIGHT"
            elif self.match(TokenType.INNER):
                self.consume(TokenType.INNER)

            self.consume(TokenType.JOIN)
            j_table = self.parse_table_or_column_name("joined table name")
            self.consume(TokenType.ON, "Expected 'on' for join condition", f"Use: join \"{j_table}\" on table1.id is table2.fk_id")
            left_col = self.parse_table_or_column_name("left join column")
            self.consume(TokenType.IS if self.match(TokenType.IS) else TokenType.EQUALS)
            right_col = self.parse_table_or_column_name("right join column")
            joins.append(JoinNode(join_type=join_type, table_name=j_table, left_col=left_col, right_col=right_col))

        # WHERE
        where = None
        self.skip_silent_words()
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        # ORDER BY / SORTED BY
        order_by = None
        self.skip_silent_words()
        if self.match(TokenType.ORDER, TokenType.SORTED):
            self.consume(self.current_token().type)
            self.skip_silent_words()
            if self.match(TokenType.BY):
                self.consume(TokenType.BY)
                self.skip_silent_words()
            field_name = self.parse_table_or_column_name("order by field")
            direction = "ASC"
            self.skip_silent_words()
            if self.match(TokenType.DESCENDING, TokenType.DESC, TokenType.HIGHEST):
                self.pos += 1
                if self.match(TokenType.FIRST):
                    self.consume(TokenType.FIRST)
                direction = "DESC"
            elif self.match(TokenType.ASCENDING, TokenType.ASC, TokenType.LOWEST):
                self.pos += 1
                if self.match(TokenType.FIRST):
                    self.consume(TokenType.FIRST)
                direction = "ASC"
            order_by = OrderByNode(field=field_name, direction=direction)

        # LIMIT & OFFSET
        self.skip_silent_words()
        offset = None
        if self.match(TokenType.LIMIT):
            self.consume(TokenType.LIMIT)
            self.skip_silent_words()
            if self.match(TokenType.TO):
                self.consume(TokenType.TO)
                self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer number after 'limit'")
            limit = int(num_tok.value)

        self.skip_silent_words()
        if self.match(TokenType.OFFSET):
            self.consume(TokenType.OFFSET)
            self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer number after 'offset'")
            offset = int(num_tok.value)

        hints: Dict[str, Any] = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()
        elif self.match(TokenType.COLON):
            self.consume(TokenType.COLON)
            if self.match(TokenType.NEWLINE):
                self.consume(TokenType.NEWLINE)
            if self.match(TokenType.INDENT):
                self.consume(TokenType.INDENT)
                while self.match(TokenType.HINT):
                    hints.update(self.parse_hints_dict())
                    if self.match(TokenType.NEWLINE):
                        self.consume(TokenType.NEWLINE)
                if self.match(TokenType.DEDENT):
                    self.consume(TokenType.DEDENT)

        return SelectNode(
            fields=fields,
            table_name=table_name,
            joins=joins,
            where=where,
            order_by=order_by,
            limit=limit,
            offset=offset,
            distinct=distinct,
            hints=hints
        )

    def parse_select_field(self) -> Any:
        self.skip_silent_words()
        tok = self.current_token()
        # Aggregate functions: count(id), avg(points), etc.
        if self.match(TokenType.COUNT, TokenType.SUM, TokenType.AVG, TokenType.MIN, TokenType.MAX):
            func_name = tok.value.upper()
            self.pos += 1
            self.consume(TokenType.LPAREN, f"Expected '(' after {func_name}")
            arg = "*" if self.match(TokenType.ALL, TokenType.STAR) else self.parse_table_or_column_name("aggregate argument")
            if arg == "*":
                self.pos += 1
            self.consume(TokenType.RPAREN, f"Expected ')' closing {func_name}")
            return FunctionCallNode(name=func_name, arguments=[arg])
        return self.parse_table_or_column_name("field name")

    # -------------------------------------------------------------
    # 3B. TOP-LEVEL COUNT QUERY (Direct English Count)
    # -------------------------------------------------------------
    def parse_count_query(self) -> SelectNode:
        self.consume(TokenType.COUNT)
        self.skip_silent_words()
        if self.match(TokenType.ALL, TokenType.STAR):
            self.consume(self.current_token().type)
            self.skip_silent_words()

        if self.match(TokenType.FROM, TokenType.IN):
            self.consume(self.current_token().type)
        else:
            raise ParserError("Expected 'from' or 'in' in count statement", self.current_token(),
                              "Use: count records in \"table_name\" where ...")
        self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")
        self.skip_silent_words()

        where = None
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return SelectNode(
            fields=[FunctionCallNode(name="COUNT", arguments=["*"])],
            table_name=table_name,
            where=where,
            hints=hints
        )

    def parse_standalone_hint(self) -> HintNode:
        return HintNode(hints=self.parse_hints_dict())

    # -------------------------------------------------------------
    # 4. UPDATE / CHANGE / IN <TABLE> UPDATE/CHANGE/SET
    # -------------------------------------------------------------
    def parse_update(self) -> UpdateNode:
        self.consume(self.current_token().type)  # UPDATE or CHANGE
        self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")
        self.skip_silent_words()
        if self.match(TokenType.SET):
            self.consume(TokenType.SET)
            self.skip_silent_words()

        assignments: Dict[str, Any] = {}
        k, v = self.parse_key_value_assignment()
        assignments[k] = v
        while self.match(TokenType.COMMA, TokenType.AND):
            self.consume(self.current_token().type)
            self.skip_silent_words()
            k, v = self.parse_key_value_assignment()
            assignments[k] = v

        where = None
        self.skip_silent_words()
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return UpdateNode(table_name=table_name, assignments=assignments, where=where, hints=hints)

    def parse_in_statement(self) -> ASTNode:
        self.consume(TokenType.IN)
        self.skip_silent_words()
        if self.match(TokenType.TABLE):
            self.consume(TokenType.TABLE)
            self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")
        self.skip_silent_words()

        if self.match(TokenType.UPDATE, TokenType.CHANGE, TokenType.SET):
            self.consume(self.current_token().type)
            self.skip_silent_words()
            if self.match(TokenType.SET):
                self.consume(TokenType.SET)
                self.skip_silent_words()

            assignments: Dict[str, Any] = {}
            k, v = self.parse_key_value_assignment()
            assignments[k] = v
            while self.match(TokenType.COMMA, TokenType.AND):
                self.consume(self.current_token().type)
                self.skip_silent_words()
                k, v = self.parse_key_value_assignment()
                assignments[k] = v

            where = None
            self.skip_silent_words()
            if self.match(TokenType.WHERE):
                self.consume(TokenType.WHERE)
                self.skip_silent_words()
                where = self.parse_expression()

            hints = {}
            self.skip_silent_words()
            if self.match(TokenType.HINT):
                hints = self.parse_hints_dict()

            return UpdateNode(table_name=table_name, assignments=assignments, where=where, hints=hints)

        raise ParserError(
            "Expected 'update', 'change', or 'set' after 'in <table_name>'",
            self.current_token(),
            "Use: in scholars change cgpa to 9.8 where name is \"aryan\";"
        )

    # -------------------------------------------------------------
    # 5. DELETE / REMOVE (With Safety Guard)
    # -------------------------------------------------------------
    def parse_delete(self) -> DeleteNode:
        self.consume(self.current_token().type)  # DELETE or REMOVE
        self.skip_silent_words()
        is_all = False
        if self.match(TokenType.ALL):
            self.consume(TokenType.ALL)
            is_all = True
            self.skip_silent_words()

        if self.match(TokenType.FROM, TokenType.IN):
            self.consume(self.current_token().type)
        else:
            raise ParserError("Expected 'from' or 'in' after delete", self.current_token(), "Use: delete from \"table_name\" where ...")
        self.skip_silent_words()
        table_name = self.parse_table_or_column_name("table name")

        where = None
        self.skip_silent_words()
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        confirmation_token = None
        self.skip_silent_words()
        if self.match(TokenType.CONFIRMED, TokenType.CONFIRM):
            confirmation_token = self.parse_confirmation(f"delete all from {table_name}", f"delete all from {table_name} confirmed")

        # 🛡️ SAFETY CHECK: Unconstrained delete without WHERE must have ALL + CONFIRMED/CONFIRM
        if where is None and not is_all and confirmation_token is None:
            raise ParserError(
                f"Destructive operation: Unconstrained delete on table '{table_name}' is blocked.",
                self.current_token(),
                f"To purge all rows safely, use: delete all from \"{table_name}\" confirmed or provide a 'where' clause."
            )

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return DeleteNode(table_name=table_name, is_all=is_all, where=where, confirmation_token=confirmation_token, hints=hints)

    # -------------------------------------------------------------
    # 6. DROP TABLE & DROP COLUMN (With Safety Guard)
    # -------------------------------------------------------------
    def parse_drop_table(self) -> ASTNode:
        self.consume(TokenType.DROP)
        
        # Support: drop column <col> from <table> confirmed
        if self.match(TokenType.COLUMN):
            self.consume(TokenType.COLUMN)
            col_name = self.parse_table_or_column_name("column name")
            self.consume(TokenType.FROM, "Expected 'from' after column name", f"Use: drop column {col_name} from <table_name> confirmed")
            table_name = self.parse_table_or_column_name("table name")
            confirm_tok = self.parse_confirmation(f"drop column {col_name} from {table_name}", f"drop column {col_name} from {table_name} confirmed")
            return AlterTableNode(table_name=table_name, action="DROP_COLUMN", drop_column=col_name, confirmation_token=confirm_tok)

        # Support: drop table <table_name> [from <db>] confirmed
        if self.match(TokenType.TABLE):
            self.consume(TokenType.TABLE)
        table_name = self.parse_table_or_column_name("table name")

        # Optional: from <database/schema>
        if self.match(TokenType.FROM):
            self.consume(TokenType.FROM)
            schema_name = self.parse_table_or_column_name("schema/database name")
            table_name = f"{schema_name}.{table_name}"

        confirm_tok = self.parse_confirmation(f"drop table {table_name}", f"drop table {table_name} confirmed")
        return DropTableNode(table_name=table_name, confirmation_token=confirm_tok)

    # -------------------------------------------------------------
    # 7. TRUNCATE TABLE (With Safety Guard)
    # -------------------------------------------------------------
    def parse_truncate_table(self) -> TruncateTableNode:
        self.consume(TokenType.TRUNCATE)
        if self.match(TokenType.TABLE):
            self.consume(TokenType.TABLE)
        table_name = self.parse_table_or_column_name("table name")
        confirm_tok = self.parse_confirmation(f"truncate table {table_name}", f"truncate table {table_name} confirmed")
        return TruncateTableNode(table_name=table_name, confirmation_token=confirm_tok)

    # -------------------------------------------------------------
    # 8. ALTER TABLE (With Safety Guard on Drop Column)
    # -------------------------------------------------------------
    def parse_alter_table(self) -> AlterTableNode:
        self.consume(TokenType.ALTER)
        self.consume(TokenType.TABLE)
        table_name = self.parse_table_or_column_name("table name")

        if self.match(TokenType.ADD):
            self.consume(TokenType.ADD)
            if self.match(TokenType.COLUMN):
                self.consume(TokenType.COLUMN)
            col_def = self.parse_column_def()
            return AlterTableNode(table_name=table_name, action="ADD_COLUMN", column_def=col_def)

        elif self.match(TokenType.DROP):
            self.consume(TokenType.DROP)
            if self.match(TokenType.COLUMN):
                self.consume(TokenType.COLUMN)
            col_name = self.parse_table_or_column_name("column name")
            confirm_tok = self.parse_confirmation(f"drop column {col_name} from {table_name}", f"alter table {table_name} drop column {col_name} confirmed")
            return AlterTableNode(
                table_name=table_name,
                action="DROP_COLUMN",
                drop_column=col_name,
                confirmation_token=confirm_tok
            )
        else:
            raise ParserError("Expected 'add' or 'drop' in alter table statement", self.current_token())

    def parse_confirmation(self, op_desc: str, example_cmd: str) -> str:
        """Enforces mandatory confirmation for destructive database actions."""
        if self.match(TokenType.CONFIRMED):
            self.consume(TokenType.CONFIRMED)
            return "CONFIRMED"
        elif self.match(TokenType.CONFIRM):
            self.consume(TokenType.CONFIRM)
            if self.match(TokenType.STRING_LITERAL):
                tok = self.consume(TokenType.STRING_LITERAL)
                return str(tok.value)
            return "CONFIRMED"
        else:
            raise ParserError(
                f"Destructive operation: '{op_desc}' is permanently blocked without confirmation.",
                self.current_token(),
                f"To execute safely, append 'confirmed' at the end.\n  Example: {example_cmd}"
            )

    # -------------------------------------------------------------
    # Helper Expressions & Literals
    # -------------------------------------------------------------
    def parse_expression(self) -> Any:
        return self.parse_logical_or()

    def parse_logical_or(self) -> Any:
        expr = self.parse_logical_and()
        while self.match(TokenType.OR):
            self.consume(TokenType.OR)
            right = self.parse_logical_and()
            expr = BinaryOpNode(left=expr, operator="OR", right=right)
        return expr

    def parse_logical_and(self) -> Any:
        expr = self.parse_comparison()
        while self.match(TokenType.AND):
            self.consume(TokenType.AND)
            right = self.parse_comparison()
            expr = BinaryOpNode(left=expr, operator="AND", right=right)
        return expr

    def parse_comparison(self) -> Any:
        self.skip_silent_words()
        expr = self.parse_additive()
        self.skip_silent_words()

        # Natural English comparisons
        if self.match(TokenType.IS):
            self.consume(TokenType.IS)
            self.skip_silent_words()
            if self.match(TokenType.NOT):
                self.consume(TokenType.NOT)
                self.skip_silent_words()
                if self.match(TokenType.EQUAL, TokenType.EQUALS):
                    self.consume(self.current_token().type)
                    if self.match(TokenType.TO):
                        self.consume(TokenType.TO)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="!=", right=right)

            elif self.match(TokenType.GREATER, TokenType.MORE, TokenType.ABOVE):
                self.consume(self.current_token().type)
                if self.match(TokenType.THAN):
                    self.consume(TokenType.THAN)
                self.skip_silent_words()
                if self.match(TokenType.OR):
                    self.consume(TokenType.OR)
                    if self.match(TokenType.EQUAL, TokenType.EQUALS):
                        self.consume(self.current_token().type)
                        if self.match(TokenType.TO):
                            self.consume(TokenType.TO)
                    right = self.parse_additive()
                    return BinaryOpNode(left=expr, operator=">=", right=right)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator=">", right=right)

            elif self.match(TokenType.AT):
                self.consume(TokenType.AT)
                if self.match(TokenType.LEAST):
                    self.consume(TokenType.LEAST)
                    right = self.parse_additive()
                    return BinaryOpNode(left=expr, operator=">=", right=right)
                elif self.match(TokenType.MOST):
                    self.consume(TokenType.MOST)
                    right = self.parse_additive()
                    return BinaryOpNode(left=expr, operator="<=", right=right)

            elif self.match(TokenType.LESS, TokenType.UNDER, TokenType.BELOW):
                self.consume(self.current_token().type)
                if self.match(TokenType.THAN):
                    self.consume(TokenType.THAN)
                self.skip_silent_words()
                if self.match(TokenType.OR):
                    self.consume(TokenType.OR)
                    if self.match(TokenType.EQUAL, TokenType.EQUALS):
                        self.consume(self.current_token().type)
                        if self.match(TokenType.TO):
                            self.consume(TokenType.TO)
                    right = self.parse_additive()
                    return BinaryOpNode(left=expr, operator="<=", right=right)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="<", right=right)

            elif self.match(TokenType.EQUAL, TokenType.EQUALS):
                self.consume(self.current_token().type)
                if self.match(TokenType.TO):
                    self.consume(TokenType.TO)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="=", right=right)

            else:
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="IS", right=right)

        elif self.match(TokenType.AT):
            self.consume(TokenType.AT)
            if self.match(TokenType.LEAST):
                self.consume(TokenType.LEAST)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator=">=", right=right)
            elif self.match(TokenType.MOST):
                self.consume(TokenType.MOST)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="<=", right=right)

        elif self.match(TokenType.GREATER, TokenType.MORE, TokenType.ABOVE):
            self.consume(self.current_token().type)
            if self.match(TokenType.THAN):
                self.consume(TokenType.THAN)
            self.skip_silent_words()
            if self.match(TokenType.OR):
                self.consume(TokenType.OR)
                if self.match(TokenType.EQUAL, TokenType.EQUALS):
                    self.consume(self.current_token().type)
                    if self.match(TokenType.TO):
                        self.consume(TokenType.TO)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator=">=", right=right)
            right = self.parse_additive()
            return BinaryOpNode(left=expr, operator=">", right=right)

        elif self.match(TokenType.LESS, TokenType.UNDER, TokenType.BELOW):
            self.consume(self.current_token().type)
            if self.match(TokenType.THAN):
                self.consume(TokenType.THAN)
            self.skip_silent_words()
            if self.match(TokenType.OR):
                self.consume(TokenType.OR)
                if self.match(TokenType.EQUAL, TokenType.EQUALS):
                    self.consume(self.current_token().type)
                    if self.match(TokenType.TO):
                        self.consume(TokenType.TO)
                right = self.parse_additive()
                return BinaryOpNode(left=expr, operator="<=", right=right)
            right = self.parse_additive()
            return BinaryOpNode(left=expr, operator="<", right=right)

        elif self.match(TokenType.EQUALS):
            self.consume(TokenType.EQUALS)
            right = self.parse_additive()
            return BinaryOpNode(left=expr, operator="=", right=right)

        op_map = {
            TokenType.EQUALS: "=",
            TokenType.NOT_EQUALS: "!=",
            TokenType.GT: ">",
            TokenType.GTE: ">=",
            TokenType.LT: "<",
            TokenType.LTE: "<=",
            TokenType.IS: "IS",
            TokenType.LIKE: "LIKE",
            TokenType.IN: "IN",
            TokenType.BETWEEN: "BETWEEN"
        }
        if self.current_token().type in op_map:
            op_tok = self.current_token()
            self.pos += 1
            right = self.parse_additive()
            return BinaryOpNode(left=expr, operator=op_map[op_tok.type], right=right)
        return expr

    def parse_additive(self) -> Any:
        expr = self.parse_primary()
        while self.match(TokenType.PLUS, TokenType.MINUS):
            op = self.consume(self.current_token().type).value
            right = self.parse_primary()
            expr = BinaryOpNode(left=expr, operator=op, right=right)
        return expr

    def parse_primary(self) -> Any:
        self.skip_silent_words()
        tok = self.current_token()
        if self.match(TokenType.NUMBER_LITERAL):
            self.consume(TokenType.NUMBER_LITERAL)
            return LiteralNode(value=tok.value, literal_type="number")
        elif self.match(TokenType.STRING_LITERAL):
            self.consume(TokenType.STRING_LITERAL)
            return LiteralNode(value=tok.value, literal_type="string")
        elif self.match(TokenType.BOOLEAN_LITERAL):
            self.consume(TokenType.BOOLEAN_LITERAL)
            return LiteralNode(value=tok.value, literal_type="boolean")
        elif self.match(TokenType.NULL_LITERAL):
            self.consume(TokenType.NULL_LITERAL)
            return LiteralNode(value=None, literal_type="null")
        elif self.match(TokenType.IDENTIFIER):
            self.consume(TokenType.IDENTIFIER)
            name = str(tok.value)
            if self.match(TokenType.DOT):
                self.consume(TokenType.DOT)
                sub_tok = self.consume(TokenType.IDENTIFIER, "Expected column name after '.'")
                name = f"{name}.{sub_tok.value}"
            return IdentifierNode(name=name)
        elif self.match(TokenType.LPAREN):
            self.consume(TokenType.LPAREN)
            expr = self.parse_expression()
            self.consume(TokenType.RPAREN, "Expected ')'")
            return expr
        else:
            raise ParserError(f"Unexpected token in expression: '{tok.value}'", tok)

    def parse_literal_or_constant(self) -> Any:
        self.skip_silent_words()
        tok = self.current_token()
        if self.match(TokenType.NUMBER_LITERAL, TokenType.STRING_LITERAL, TokenType.BOOLEAN_LITERAL, TokenType.NULL_LITERAL):
            self.pos += 1
            return tok.value
        elif self.match(TokenType.IDENTIFIER):
            self.pos += 1
            return str(tok.value)
        raise ParserError(f"Expected literal default value, found '{tok.value}'", tok)

    def parse_table_or_column_name(self, context: str = "identifier") -> str:
        self.skip_silent_words()
        # Optional noise words before table or column
        if self.match(TokenType.TABLE):
            self.consume(TokenType.TABLE)
            self.skip_silent_words()
        elif self.match(TokenType.COLUMN):
            self.consume(TokenType.COLUMN)
            self.skip_silent_words()

        tok = self.current_token()
        if self.match(TokenType.STRING_LITERAL, TokenType.IDENTIFIER):
            self.pos += 1
            name = str(tok.value)
            # Dot notation support (table.column)
            if self.match(TokenType.DOT):
                self.consume(TokenType.DOT)
                sub_tok = self.consume(TokenType.IDENTIFIER, "Expected column name after '.'")
                name = f"{name}.{sub_tok.value}"
            return name
        raise ParserError(f"Expected {context}, but found '{tok.value}'", tok,
                          f"Provide a valid {context} (e.g. \"users\" or users).")
