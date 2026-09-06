"""Parser for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

from typing import List, Optional, Any, Dict
from enlngdb.tokens import Token, TokenType
from enlngdb.ast_nodes import (
    ProgramNode, DomainHeaderNode, DisplayNode, CreateTableNode, ColumnDefNode,
    InsertRecordNode, FindRecordsNode, UpdateRecordsNode, DeleteRecordsNode,
    CountRecordsNode, OpenDatabaseNode, SaveDatabaseNode, HintNode, OrderByNode,
    BinaryOpNode, UnaryOpNode, IdentifierNode, LiteralNode, ASTNode
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
        while self.match(TokenType.NEWLINE, TokenType.SEMICOLON):
            self.pos += 1

    def skip_silent_words(self):
        """Skips conversational filler words like 'the', 'a', 'an', 'please', 'records', 'rows', etc."""
        silent_types = {
            TokenType.THE, TokenType.A, TokenType.AN,
            TokenType.PLEASE, TokenType.KINDLY,
            TokenType.RECORD, TokenType.RECORDS,
            TokenType.ROW, TokenType.ROWS,
            TokenType.ENTRY, TokenType.ENTRIES,
            TokenType.DATA, TokenType.ITEM, TokenType.ITEMS
        }
        delimiters = {
            TokenType.WITH, TokenType.WHERE, TokenType.SET,
            TokenType.COMMA, TokenType.SEMICOLON, TokenType.NEWLINE,
            TokenType.EOF, TokenType.COLON, TokenType.EQUALS, TokenType.IS
        }
        while self.match(*silent_types):
            if self.peek_token(1).type in delimiters:
                break
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
        while self.match(TokenType.INDENT, TokenType.DEDENT):
            self.pos += 1

        header = None
        if self.match(TokenType.TYPE):
            self.consume(TokenType.TYPE)
            if self.match(TokenType.ENLNGDB, TokenType.ENLGDB):
                dom_tok = self.consume(self.current_token().type)
                header = DomainHeaderNode(domain=str(dom_tok.value))
                self.skip_newlines()

        statements: List[ASTNode] = []
        while not self.match(TokenType.EOF):
            self.skip_newlines()
            while self.match(TokenType.INDENT, TokenType.DEDENT):
                self.pos += 1
            if self.match(TokenType.EOF):
                break
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
            while self.match(TokenType.SEMICOLON):
                self.consume(TokenType.SEMICOLON)
            self.skip_newlines()

        return ProgramNode(header=header, statements=statements)

    def parse_statement(self) -> ASTNode:
        self.skip_silent_words()
        tok = self.current_token()

        # DISPLAY ...
        if self.match(TokenType.DISPLAY):
            return self.parse_display()

        # HINT ...
        elif self.match(TokenType.HINT):
            return HintNode(hints=self.parse_hints_dict())

        # OPEN / CONNECT DATABASE ...
        elif self.match(TokenType.OPEN, TokenType.CONNECT):
            return self.parse_open_database()

        # SAVE DATABASE ...
        elif self.match(TokenType.SAVE):
            if self.peek_token(1).type == TokenType.DATABASE:
                return self.parse_save_database()
            return self.parse_insert()

        # CREATE TABLE ...
        elif self.match(TokenType.CREATE):
            return self.parse_create_table()

        # INSERT INTO / PUT ...
        elif self.match(TokenType.INSERT, TokenType.PUT):
            return self.parse_insert()

        # FIND / FETCH / GET / SELECT / SHOW ...
        elif self.match(TokenType.FIND, TokenType.FETCH, TokenType.GET, TokenType.SELECT, TokenType.SHOW):
            return self.parse_find()

        # UPDATE / CHANGE ...
        elif self.match(TokenType.UPDATE, TokenType.CHANGE):
            return self.parse_update()

        # COUNT ...
        elif self.match(TokenType.COUNT):
            return self.parse_count()

        # DELETE / REMOVE ...
        elif self.match(TokenType.DELETE, TokenType.REMOVE):
            return self.parse_delete()

        else:
            raise ParserError(
                f"Unexpected enlngdb statement starting with '{tok.value}'",
                tok,
                "Valid statements: create table, insert record, find records, update records, count records, delete records, display, hint."
            )

    # -------------------------------------------------------------
    # 1. DISPLAY
    # -------------------------------------------------------------
    def parse_display(self) -> DisplayNode:
        self.consume(TokenType.DISPLAY)
        msg_expr = self.parse_expression()
        return DisplayNode(message=msg_expr)

    # -------------------------------------------------------------
    # 2. DATABASE OPEN & SAVE
    # -------------------------------------------------------------
    def parse_open_database(self) -> OpenDatabaseNode:
        self.consume(self.current_token().type)  # OPEN or CONNECT
        if self.match(TokenType.TO):
            self.consume(TokenType.TO)
        if self.match(TokenType.DATABASE):
            self.consume(TokenType.DATABASE)
        path_tok = self.consume(TokenType.STRING_LITERAL, "Expected database file path string")
        return OpenDatabaseNode(db_path=str(path_tok.value))

    def parse_save_database(self) -> SaveDatabaseNode:
        self.consume(TokenType.SAVE)
        self.consume(TokenType.DATABASE)
        if self.match(TokenType.TO):
            self.consume(TokenType.TO)
        path_tok = self.consume(TokenType.STRING_LITERAL, "Expected database file path string")
        return SaveDatabaseNode(db_path=str(path_tok.value))

    # -------------------------------------------------------------
    # 3. CREATE TABLE
    # -------------------------------------------------------------
    def parse_create_table(self) -> CreateTableNode:
        self.consume(TokenType.CREATE)
        self.consume(TokenType.TABLE)
        self.skip_silent_words()

        if_not_exists = False
        if self.match(TokenType.IF):
            self.consume(TokenType.IF)
            if self.match(TokenType.NOT):
                self.consume(TokenType.NOT)
            if self.match(TokenType.EXISTS):
                self.consume(TokenType.EXISTS)
            if_not_exists = True
            self.skip_silent_words()

        table_name = self.parse_identifier_or_string("table name")
        self.skip_silent_words()

        self.consume(TokenType.WITH, "Expected 'with' after table name", "Use: create table accounts with id, name, balance")
        columns: List[ColumnDefNode] = []
        table_hints: Dict[str, Any] = {}

        if self.match(TokenType.COLON):
            # Indented block schema definition
            self.consume(TokenType.COLON)
            self.skip_newlines()
            self.consume(TokenType.INDENT, "Expected indented column definitions")
            while not self.match(TokenType.DEDENT, TokenType.EOF):
                self.skip_newlines()
                if self.match(TokenType.DEDENT, TokenType.EOF):
                    break
                if self.match(TokenType.HINT):
                    table_hints.update(self.parse_hints_dict())
                else:
                    col_def = self.parse_indented_column_def()
                    columns.append(col_def)
                self.skip_newlines()
            self.consume(TokenType.DEDENT)
        else:
            # Inline comma-separated field list: with id, account_holder, balance, status
            while True:
                self.skip_silent_words()
                col_name = self.parse_identifier_or_string("column name")
                columns.append(ColumnDefNode(name=col_name, data_type="ANY"))
                self.skip_silent_words()
                if self.match(TokenType.COMMA):
                    self.consume(TokenType.COMMA)
                else:
                    break

        return CreateTableNode(table_name=table_name, columns=columns, if_not_exists=if_not_exists, hints=table_hints)

    def parse_indented_column_def(self) -> ColumnDefNode:
        col_name = self.parse_identifier_or_string("column name")
        data_type = "ANY"
        if self.match(TokenType.AS):
            self.consume(TokenType.AS)
            type_tok = self.current_token()
            data_type = str(type_tok.value).upper()
            self.pos += 1

        is_primary = False
        col_hints = {}
        while not self.match(TokenType.NEWLINE, TokenType.DEDENT, TokenType.EOF):
            if self.match(TokenType.PRIMARY):
                self.consume(TokenType.PRIMARY)
                if self.match(TokenType.KEY):
                    self.consume(TokenType.KEY)
                is_primary = True
            elif self.match(TokenType.HINT):
                col_hints.update(self.parse_hints_dict())
            else:
                break

        return ColumnDefNode(name=col_name, data_type=data_type, is_primary_key=is_primary, hints=col_hints)

    # -------------------------------------------------------------
    # 4. INSERT RECORD
    # -------------------------------------------------------------
    def parse_insert(self) -> InsertRecordNode:
        self.consume(self.current_token().type)  # INSERT, PUT, SAVE
        self.skip_silent_words()
        if self.match(TokenType.INTO):
            self.consume(TokenType.INTO)
        self.skip_silent_words()
        table_name = self.parse_identifier_or_string("table name")
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
            # Inline key-value pairs: with id 101, account_holder "Bibhu", balance 85000
            while True:
                self.skip_silent_words()
                k, v = self.parse_key_value_assignment()
                values[k] = v
                self.skip_silent_words()
                if self.match(TokenType.COMMA, TokenType.AND):
                    self.consume(self.current_token().type)
                else:
                    break

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return InsertRecordNode(table_name=table_name, values=values, hints=hints)

    def parse_key_value_assignment(self) -> tuple:
        self.skip_silent_words()
        key = self.parse_identifier_or_string("field name")
        self.skip_silent_words()
        if self.match(TokenType.COLON, TokenType.EQUALS, TokenType.AS, TokenType.TO, TokenType.IS):
            self.consume(self.current_token().type)

        self.skip_silent_words()
        val = self.parse_expression()
        return key, val

    # -------------------------------------------------------------
    # 5. FIND RECORDS
    # -------------------------------------------------------------
    def parse_find(self) -> FindRecordsNode:
        self.consume(self.current_token().type)  # FIND, FETCH, GET, SELECT, SHOW
        self.skip_silent_words()

        limit = None
        if self.match(TokenType.TOP):
            self.consume(TokenType.TOP)
            self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected integer after 'top'")
            limit = int(num_tok.value)
            self.skip_silent_words()

        fields: List[str] = []
        if self.match(TokenType.ALL):
            self.consume(TokenType.ALL)
            fields = ["*"]
            self.skip_silent_words()
        elif self.match(TokenType.FROM, TokenType.IN):
            fields = ["*"]
        else:
            while True:
                self.skip_silent_words()
                f_name = self.parse_identifier_or_string("field name")
                fields.append(f_name)
                self.skip_silent_words()
                if self.match(TokenType.COMMA, TokenType.AND):
                    self.consume(self.current_token().type)
                else:
                    break

        if self.match(TokenType.FROM, TokenType.IN):
            self.consume(self.current_token().type)
        else:
            raise ParserError("Expected 'from' or 'in' after field list in find statement", self.current_token())

        self.skip_silent_words()
        table_name = self.parse_identifier_or_string("table name")
        self.skip_silent_words()

        where = None
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        order_by = None
        self.skip_silent_words()
        if self.match(TokenType.ORDER, TokenType.SORTED):
            self.consume(self.current_token().type)
            self.skip_silent_words()
            if self.match(TokenType.BY):
                self.consume(TokenType.BY)
                self.skip_silent_words()
            f_name = self.parse_identifier_or_string("sort field")
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
            order_by = OrderByNode(field=f_name, direction=direction)

        self.skip_silent_words()
        offset = None
        if self.match(TokenType.LIMIT):
            self.consume(TokenType.LIMIT)
            self.skip_silent_words()
            if self.match(TokenType.TO):
                self.consume(TokenType.TO)
                self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected limit count")
            limit = int(num_tok.value)

        self.skip_silent_words()
        if self.match(TokenType.OFFSET):
            self.consume(TokenType.OFFSET)
            self.skip_silent_words()
            num_tok = self.consume(TokenType.NUMBER_LITERAL, "Expected offset count")
            offset = int(num_tok.value)

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return FindRecordsNode(
            table_name=table_name,
            fields=fields,
            where=where,
            order_by=order_by,
            limit=limit,
            offset=offset,
            hints=hints
        )

    # -------------------------------------------------------------
    # 6. UPDATE RECORDS
    # -------------------------------------------------------------
    def parse_update(self) -> UpdateRecordsNode:
        self.consume(self.current_token().type)  # UPDATE or CHANGE
        self.skip_silent_words()
        if self.match(TokenType.IN):
            self.consume(TokenType.IN)
        self.skip_silent_words()
        table_name = self.parse_identifier_or_string("table name")
        self.skip_silent_words()

        self.consume(TokenType.SET, "Expected 'set' in update statement", "Use: update accounts set status = 'VIP' where ...")
        self.skip_silent_words()

        assignments: Dict[str, Any] = {}
        while True:
            self.skip_silent_words()
            k, v = self.parse_key_value_assignment()
            assignments[k] = v
            self.skip_silent_words()
            if self.match(TokenType.COMMA, TokenType.AND):
                self.consume(self.current_token().type)
            else:
                break

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

        return UpdateRecordsNode(table_name=table_name, assignments=assignments, where=where, hints=hints)

    # -------------------------------------------------------------
    # 7. COUNT RECORDS
    # -------------------------------------------------------------
    def parse_count(self) -> CountRecordsNode:
        self.consume(TokenType.COUNT)
        self.skip_silent_words()
        if self.match(TokenType.ALL):
            self.consume(TokenType.ALL)
            self.skip_silent_words()

        if self.match(TokenType.IN, TokenType.FROM):
            self.consume(self.current_token().type)
        else:
            raise ParserError("Expected 'in' or 'from' in count statement", self.current_token(),
                              "Use: count records in accounts where ...")

        self.skip_silent_words()
        table_name = self.parse_identifier_or_string("table name")
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

        return CountRecordsNode(table_name=table_name, where=where, hints=hints)

    # -------------------------------------------------------------
    # 8. DELETE RECORDS
    # -------------------------------------------------------------
    def parse_delete(self) -> DeleteRecordsNode:
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
            raise ParserError("Expected 'from' or 'in' in delete statement", self.current_token())

        self.skip_silent_words()
        table_name = self.parse_identifier_or_string("table name")

        where = None
        self.skip_silent_words()
        if self.match(TokenType.WHERE):
            self.consume(TokenType.WHERE)
            self.skip_silent_words()
            where = self.parse_expression()

        confirm_tok = None
        self.skip_silent_words()
        if self.match(TokenType.CONFIRMED, TokenType.CONFIRM):
            self.consume(self.current_token().type)
            confirm_tok = "CONFIRMED"

        hints = {}
        self.skip_silent_words()
        if self.match(TokenType.HINT):
            hints = self.parse_hints_dict()

        return DeleteRecordsNode(table_name=table_name, where=where, is_all=is_all, confirmation_token=confirm_tok, hints=hints)

    # -------------------------------------------------------------
    # Expressions, Comparators & Primary Literals
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
                return BinaryOpNode(left=expr, operator="=", right=right)

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
            TokenType.LIKE: "LIKE"
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
                sub_tok = self.consume(TokenType.IDENTIFIER, "Expected sub-field after '.'")
                name = f"{name}.{sub_tok.value}"
            return IdentifierNode(name=name)
        elif self.match(TokenType.LPAREN):
            self.consume(TokenType.LPAREN)
            expr = self.parse_expression()
            self.consume(TokenType.RPAREN, "Expected ')'")
            return expr
        else:
            raise ParserError(f"Unexpected token in expression: '{tok.value}'", tok)

    def parse_identifier_or_string(self, context: str = "identifier") -> str:
        self.skip_silent_words()
        if self.match(TokenType.TABLE):
            self.consume(TokenType.TABLE)
            self.skip_silent_words()

        tok = self.current_token()
        if self.match(TokenType.STRING_LITERAL, TokenType.IDENTIFIER):
            self.pos += 1
            return str(tok.value)
        elif tok.type not in (TokenType.WITH, TokenType.WHERE, TokenType.FROM, TokenType.INTO, TokenType.SET, TokenType.EOF, TokenType.SEMICOLON, TokenType.NEWLINE, TokenType.COLON, TokenType.COMMA) and isinstance(tok.value, str):
            self.pos += 1
            return str(tok.value)
        raise ParserError(f"Expected {context}, but found '{tok.value}'", tok)
