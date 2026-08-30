/**
 * Official VS Code Extension for EnLang.
 * Provides commands for Running, Building, Checking Syntax, and Consulting Enlang AI.
 */

const vscode = require('vscode');

let enlangTerminal = null;

function getTerminal() {
    if (!enlangTerminal || enlangTerminal.exitStatus !== undefined) {
        enlangTerminal = vscode.window.createTerminal("EnLang");
    }
    return enlangTerminal;
}

function activate(context) {
    // 1. Run Current File
    let runDisposable = vscode.commands.registerCommand('enlang.runFile', function () {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('No active EnLang file to run.');
            return;
        }
        const filePath = editor.document.fileName;
        const terminal = getTerminal();
        terminal.show();
        terminal.sendText(`enlang run "${filePath}"`);
    });

    // 2. Build Current File
    let buildDisposable = vscode.commands.registerCommand('enlang.buildFile', function () {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('No active EnLang file to build.');
            return;
        }
        const filePath = editor.document.fileName;
        const terminal = getTerminal();
        terminal.show();
        terminal.sendText(`enlang build "${filePath}"`);
    });

    // 3. Check Syntax / Diagnostic
    let checkDisposable = vscode.commands.registerCommand('enlang.checkFile', function () {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('No active EnLang file to check.');
            return;
        }
        const filePath = editor.document.fileName;
        const terminal = getTerminal();
        terminal.show();
        terminal.sendText(`enlang check "${filePath}"`);
    });

    // 4. Ask Enlang AI Assistant
    let aiDisposable = vscode.commands.registerCommand('enlang.askAI', async function () {
        const query = await vscode.window.showInputBox({
            prompt: 'Ask EnLang AI a question or request a code snippet:',
            placeHolder: 'e.g. How do I create a reactive score modal in enlgs?'
        });

        if (query && query.trim()) {
            const terminal = getTerminal();
            terminal.show();
            terminal.sendText(`enlang ai "${query.replace(/"/g, '\\"')}"`);
        }
    });

    // 5. Start REPL
    let replDisposable = vscode.commands.registerCommand('enlang.startRepl', function () {
        const terminal = getTerminal();
        terminal.show();
        terminal.sendText('enlang repl');
    });

    context.subscriptions.push(runDisposable);
    context.subscriptions.push(buildDisposable);
    context.subscriptions.push(checkDisposable);
    context.subscriptions.push(aiDisposable);
    context.subscriptions.push(replDisposable);
}

function deactivate() {
    if (enlangTerminal) {
        enlangTerminal.dispose();
    }
}

module.exports = {
    activate,
    deactivate
};
