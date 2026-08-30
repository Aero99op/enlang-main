"""Parser for enlgdb (Natural English SQL & Database Language)."""

from typing import List, Optional, Any, Dict
from enlgdb.tokens import Token, TokenType
from enlgdb.ast_nodes import (
    ProgramNode, DomainHeaderNode, CreateTableNode, ColumnDefNode,
    CreateDatabaseNode, UseDatabaseNode, ShowDatabasesNode, ShowTablesNode, DropDatabaseNode,
    InsertNode, SelectNode, OrderByNode, JoinNode,
    UpdateNode, DeleteNode, DropTableNode, TruncateTableNode,
    AlterTableNode, BinaryOpNode, UnaryOpNode, FunctionCallNode,
    IdentifierNode, LiteralNode, ASTNode
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
        tok = self.current_token()

        # CREATE TABLE / DATABASE ...
        if self.match(TokenType.CREATE):
            if self.peek_token(1).type == TokenType.DATABASE:
                return self.parse_create_database()
            return self.parse_create_table()

        # USE DATABASE ...
        elif self.match(TokenType.USE):
            return self.parse_use_database()

        # SHOW DATABASES / TABLES ...
        elif self.match(TokenType.SHOW):
            return self.parse_show()

        # INSERT INTO ...
        elif self.match(TokenType.INSERT):
            return self.parse_insert()

        # SELECT ...
        elif self.match(TokenType.SELECT):
            return self.parse_select()

        # UPDATE ...
        elif self.match(TokenType.UPDATE):
            return self.parse_update()

        # DELETE ...
        elif self.match(TokenType.DELETE):
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
                "Valid enlgdb statements are: 'create database', 'use database', 'show databases', 'show tables', 'create table', 'insert into', 'select', 'update', 'delete', 'drop table'."
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
        while not self.match(TokenType.DEDENT, TokenType.EOF):
            self.skip_newlines()
            if self.match(TokenType.DEDENT, TokenType.EOF):
                break

            col_def = self.parse_column_def()
            columns.append(col_def)
            self.skip_newlines()

        self.consume(TokenType.DEDENT, "Expected dedent after column block")
        return CreateTableNode(table_name=table_name, columns=columns)

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
            references_column=references_col
        )

    # -------------------------------------------------------------
    # 2. INSERT INTO
    # -------------------------------------------------------------
    def parse_insert(self) -> InsertNode:
        self.consume(TokenType.INSERT)
        self.consume(TokenType.INTO, "Expected 'into' after 'insert'", "Use: insert into \"table_name\" values:")
        table_name = self.parse_table_or_column_name("table name")
        self.consume(TokenType.VALUES, "Expected 'values:' after table name", "Use: insert into \"table_name\" values:")

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
            while self.match(TokenType.COMMA):
                self.consume(TokenType.COMMA)
                k, v = self.parse_key_value_assignment()
                values[k] = v

        return InsertNode(table_name=table_name, values=values)

    def parse_key_value_assignment(self) -> tuple:
        key = self.parse_table_or_column_name("field name")
        if self.match(TokenType.COLON):
            self.consume(TokenType.COLON)
        elif self.match(TokenType.EQUALS):
            self.consume(TokenType.EQUALS)
        elif self.match(TokenType.AS):
            self.consume(TokenType.AS)
        else:
            raise ParserError(f"Expected ':' or '=' after field '{key}'", self.current_token(),
                              f"Use: {key}: value")

        val = self.parse_expression()
        return key, val

    # -------------------------------------------------------------
    # 3. SELECT (DQL)
    # -------------------------------------------------------------
    def parse_select(self) -> SelectNode:
        self.consume(TokenType.SELECT)
        distinct = False
        if self.match(TokenType.DISTINCT):
            self.consume(TokenType.DISTINCT)
            distinct = True

        fields: List[Any] = []
        if self.match(TokenType.ALL, TokenType.STAR):
            self.pos += 1
            fields.append("*")
        else:
            fields.append(self.parse_select_field())
            while self.match(TokenType.COMMA):
                self.consume(TokenType.COMMA)
                fields.append(self.parse_select_field())

        self.consume(TokenType.FROM, "Expected 'from' in select statement", "Use: select ... from \"table_name\"")
        table_name = self.parse_table_or_column_name("table name")

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
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            where = self.parse_expression()

        # ORDER BY
        order_by = None
        if self.match(TokenType.ORDER):
            self.consume(TokenType.ORDER)
            self.consume(TokenType.BY, "Expected 'by' after 'order'", "Use: order by <field> [ascending/descending]")
            field_name = self.parse_table_or_column_name("order by field")
            direction = "ASC"
            if self.match(TokenType.DESCENDING, TokenType.DESC):
                self.pos += 1
                direction = "DESC"
            elif self.match(TokenType.ASCENDING, TokenType.ASC):
                self.pos += 1
                direction = "ASC"
            order_by = OrderByNode(field=field_name, direction=direction)

        # LIMIT & OFFSET
        limit = None
        offset = None
        if self.match(TokenType.LIMIT):
            self.consume(TokenType.LIMIT)
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer number after 'limit'")
            limit = int(num_tok.value)

        if self.match(TokenType.OFFSET):
            self.consume(TokenType.OFFSET)
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer number after 'offset'")
            offset = int(num_tok.value)

        return SelectNode(
            fields=fields,
            table_name=table_name,
            joins=joins,
            where=where,
            order_by=order_by,
            limit=limit,
            offset=offset,
            distinct=distinct
        )

    def parse_select_field(self) -> Any:
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
    # 4. UPDATE
    # -------------------------------------------------------------
    def parse_update(self) -> UpdateNode:
        self.consume(TokenType.UPDATE)
        table_name = self.parse_table_or_column_name("table name")
        self.consume(TokenType.SET, "Expected 'set' after table name", "Use: update \"table_name\" set col = val where ...")

        assignments: Dict[str, Any] = {}
        k, v = self.parse_key_value_assignment()
        assignments[k] = v
        while self.match(TokenType.COMMA):
            self.consume(TokenType.COMMA)
            k, v = self.parse_key_value_assignment()
            assignments[k] = v

        where = None
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            where = self.parse_expression()

        return UpdateNode(table_name=table_name, assignments=assignments, where=where)

    # -------------------------------------------------------------
    # 5. DELETE (With Safety Guard)
    # -------------------------------------------------------------
    def parse_delete(self) -> DeleteNode:
        self.consume(TokenType.DELETE)
        is_all = False
        if self.match(TokenType.ALL):
            self.consume(TokenType.ALL)
            is_all = True

        self.consume(TokenType.FROM, "Expected 'from' after delete", "Use: delete from \"table_name\" where ...")
        table_name = self.parse_table_or_column_name("table name")

        where = None
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            where = self.parse_expression()

        confirmation_token = None
        if self.match(TokenType.CONFIRMED, TokenType.CONFIRM):
            confirmation_token = self.parse_confirmation(f"delete all from {table_name}", f"delete all from {table_name} confirmed")

        # 🛡️ SAFETY CHECK: Unconstrained delete without WHERE must have ALL + CONFIRMED/CONFIRM
        if where is None and not is_all and confirmation_token is None:
            raise ParserError(
                f"Destructive operation: Unconstrained delete on table '{table_name}' is blocked.",
                self.current_token(),
                f"To purge all rows safely, use: delete all from \"{table_name}\" confirmed or provide a 'where' clause."
            )

        return DeleteNode(table_name=table_name, is_all=is_all, where=where, confirmation_token=confirmation_token)

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
        expr = self.parse_additive()
        op_map = {
            TokenType.EQUALS: "=",
            TokenType.NOT_EQUALS: "!=",
            TokenType.GT: ">",
            TokenType.GTE: ">=",
            TokenType.LT: "<",
            TokenType.LTE: "<=",
            TokenType.IS: "IS",
            TokenType.LIKE: "LIKE",
            TokenType.IN: "IN"
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
            # Check for dotted identifier: users.id
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
        tok = self.current_token()
        if self.match(TokenType.NUMBER_LITERAL, TokenType.STRING_LITERAL, TokenType.BOOLEAN_LITERAL, TokenType.NULL_LITERAL):
            self.pos += 1
            return tok.value
        elif self.match(TokenType.IDENTIFIER):
            self.pos += 1
            return str(tok.value)
        raise ParserError(f"Expected literal default value, found '{tok.value}'", tok)

    def parse_table_or_column_name(self, context: str = "identifier") -> str:
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
