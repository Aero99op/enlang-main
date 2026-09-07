// Compiled by Enlangg Sovereign Mobile Compiler v5.3.0

import 'package:flutter/material.dart';

void main() => runApp(const PrayasSovereignAppApp());

class PrayasSovereignAppApp extends StatelessWidget {
  const PrayasSovereignAppApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'PrayasSovereignApp',
      theme: ThemeData.dark(),
      home: const MobileScreen(),
    );
  }
}

class MobileScreen extends StatelessWidget {
  const MobileScreen({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Prayas Sovereign Mobile')),
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text('Welcome to Sovereign Mobile', style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold)),
            SizedBox(height: 12),
            const Divider(),
            SizedBox(height: 12),
            Text('Pure C Mobile Runtime with Zero Flutter / Zero Android Studio dependency.', style: TextStyle(fontSize: 14, fontWeight: FontWeight.normal)),
            SizedBox(height: 20),
            ElevatedButton(onPressed: () {}, child: Text('Open Sovereign Store')),
            ElevatedButton(onPressed: () {}, child: Text('Check System Health')),
          ],
        ),
      ),
    );
  }
}
