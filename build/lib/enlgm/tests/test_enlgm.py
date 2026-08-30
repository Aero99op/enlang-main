"""Unit tests for enlgm (Enlang Mobile to Flutter Compiler)."""

import unittest
from enlgm.compiler import compile_enlgm_source

class TestEnlgmSyntax(unittest.TestCase):

    def test_mandatory_first_line(self):
        # Missing 'in mobile:' must raise SyntaxError
        invalid_source = """
screen HomeScreen:
    title "Home"
"""
        with self.assertRaises(SyntaxError):
            compile_enlgm_source(invalid_source)

    def test_app_definition(self):
        source = """
in mobile:
    use flutter "material"
    use package "google_fonts"

    app "PrayasApp":
        theme dark
        accent color "#00f2fe"
        home screen HomeScreen

    screen HomeScreen:
        body:
            text "Hello Mobile"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("import 'package:flutter/material.dart';", dart)
        self.assertIn("import 'package:google_fonts/google_fonts.dart';", dart)
        self.assertIn("void main() => runApp(const PrayasAppApp());", dart)
        self.assertIn("class PrayasAppApp extends StatelessWidget", dart)
        self.assertIn("title: 'PrayasApp'", dart)
        self.assertIn("ThemeData.dark().copyWith", dart)
        self.assertIn("Color(0xFF00F2FE)", dart)
        self.assertIn("home: const HomeScreen()", dart)

    def test_stateless_screen_with_appbar_and_fab(self):
        source = """
in mobile:
    app "DemoApp":
        home screen MainScreen

    screen MainScreen:
        app bar:
            title "Dashboard"
            actions:
                icon button "search":
                    when tapped:
                        show toast "Searching..."

        body:
            column centered:
                text "Welcome Prayas" size 28, bold, color "#ffffff"
                spacer height 16
                button "Open Details":
                    when tapped:
                        go to DetailScreen

        floating button "add":
            when tapped:
                show snackbar "Added item!"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("class MainScreen extends StatelessWidget", dart)
        self.assertIn("appBar: AppBar(", dart)
        self.assertIn("title: Text(\"Dashboard\")", dart)
        self.assertIn("IconButton(onPressed:", dart)
        self.assertIn("floatingActionButton: FloatingActionButton(", dart)
        self.assertIn("Navigator.push(context, MaterialPageRoute(builder: (_) => const DetailScreen()));", dart)

    def test_stateful_screen_and_mutations(self):
        source = """
in mobile:
    stateful screen CounterScreen:
        create count as 0

        title "Counter App"
        body:
            column centered:
                text count size 48, bold
                row:
                    button "+":
                        when tapped:
                            increase count by 1
                    button "-":
                        when tapped:
                            decrease count by 1
                    button "Reset":
                        when tapped:
                            set count to 0
"""
        dart = compile_enlgm_source(source)
        self.assertIn("class CounterScreen extends StatefulWidget", dart)
        self.assertIn("class _CounterScreenState extends State<CounterScreen>", dart)
        self.assertIn("dynamic count = 0;", dart)
        self.assertIn("setState(() { count += 1; });", dart)
        self.assertIn("setState(() { count -= 1; });", dart)
        self.assertIn("setState(() { count = 0; });", dart)

    def test_navigation_actions(self):
        source = """
in mobile:
    screen NavDemo:
        body:
            column:
                button "Push":
                    when tapped:
                        go to ProfileScreen
                button "Back":
                    when tapped:
                        go back
                button "Replace":
                    when tapped:
                        replace screen with LoginScreen
                button "Clear":
                    when tapped:
                        go to LoginScreen and clear all
"""
        dart = compile_enlgm_source(source)
        self.assertIn("Navigator.push(context, MaterialPageRoute(builder: (_) => const ProfileScreen()));", dart)
        self.assertIn("Navigator.pop(context);", dart)
        self.assertIn("Navigator.pushReplacement(context, MaterialPageRoute(builder: (_) => const LoginScreen()));", dart)
        self.assertIn("Navigator.pushAndRemoveUntil(context, MaterialPageRoute(builder: (_) => const LoginScreen()), (_) => false);", dart)

    def test_layout_widgets(self):
        source = """
in mobile:
    screen LayoutDemo:
        body:
            scroll:
                column:
                    card elevation 6, radius 16:
                        padding 20:
                            row spaced:
                                avatar from "https://example.com/me.jpg" size 60
                                text "Prayas" bold, size 18
                    container width 300, height 150, color "#1a1a2e", radius 12:
                        center:
                            chip "Active"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("SingleChildScrollView(child:", dart)
        self.assertIn("Card(elevation: 6, shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16))", dart)
        self.assertIn("CircleAvatar(radius: 30.0, backgroundImage: NetworkImage('https://example.com/me.jpg'))", dart)
        self.assertIn("Container(width: 300, height: 150, decoration: BoxDecoration(color: const Color(0xFF1A1A2E), borderRadius: BorderRadius.circular(12))", dart)
        self.assertIn("Chip(label: Text(\"Active\"))", dart)

    def test_input_textfields(self):
        source = """
in mobile:
    stateful screen FormScreen:
        body:
            column:
                input "nameField" placeholder "Enter Name"
                input "passField" placeholder "Password" type password
"""
        dart = compile_enlgm_source(source)
        self.assertIn("final TextEditingController _nameFieldController = TextEditingController();", dart)
        self.assertIn("final TextEditingController _passFieldController = TextEditingController();", dart)
        self.assertIn("_nameFieldController.dispose();", dart)
        self.assertIn("TextField(controller: _nameFieldController, decoration: const InputDecoration(hintText: 'Enter Name'))", dart)
        self.assertIn("obscureText: true", dart)

    def test_feedback_and_dialogs(self):
        source = """
in mobile:
    screen AlertDemo:
        body:
            button "Show Alert":
                when tapped:
                    show alert "Confirm Delete":
                        confirm "Yes"
                        cancel "No"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("showDialog(context: context, builder: (ctx) => AlertDialog(", dart)
        self.assertIn("title: Text(\"Confirm Delete\")", dart)

    def test_network_calls(self):
        source = """
in mobile:
    screen NetDemo:
        body:
            button "Fetch Users":
                when tapped:
                    load from "https://api.example.com/users":
                        on success with result:
                            show toast "Loaded"
                        on failure with error:
                            show toast "Failed"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("http.get(Uri.parse(\"https://api.example.com/users\")).then((result) {", dart)
        self.assertIn("}).catchError((error) {", dart)

    def test_blueprint_custom_widgets(self):
        source = """
in mobile:
    blueprint SkillBadge:
        needs skillName, colorHex
        card radius 8:
            padding 12:
                text skillName bold

    screen SkillsScreen:
        body:
            column:
                text "Skills"
"""
        dart = compile_enlgm_source(source)
        self.assertIn("class SkillBadge extends StatelessWidget {", dart)
        self.assertIn("final dynamic skillName;", dart)
        self.assertIn("final dynamic colorHex;", dart)
        self.assertIn("const SkillBadge({this.skillName, this.colorHex}, {super.key});", dart)

    def test_raw_dart_escape(self):
        source = """
in mobile:
    screen DartScreen:
        body:
            column:
                text "Dart Native"

    write dart:
        void customHelper() {
            print("Native Dart logic");
        }
"""
        dart = compile_enlgm_source(source)
        self.assertIn("void customHelper() {", dart)
        self.assertIn("print(\"Native Dart logic\");", dart)

if __name__ == "__main__":
    unittest.main()
