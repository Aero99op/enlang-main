# Making Enlng Complete, the Simple Way

## Status

**Built and tested** in both `enlng run` and `enlng run --native`, with identical output:

- 1–7, 9, 10 and 12–15 (`when the color is:`, `if ... otherwise`, `is in`, power, taking apart, list building, inputs by name, `stop the program`, tests, bytes, web server, network, big whole numbers)
- 8: `check that x is a number` works while the program runs. Reporting a wrong kind before the program runs is **not built yet**.
- Tools: interactive mode (`enlng`) and `enlng test`

**Not built yet:**

- 11 `run together:`
- Editor help, debugger, packages, release

Every Enlng line is a short English sentence made of three kinds of words:

| Kind | What it does | Examples |
|---|---|---|
| **Action word** | Starts the sentence and says what happens | `remember`, `show`, `when`, `for`, `give`, `stop`, `check`, `run`, `read`, `write`, `send` |
| **Connector word** | Joins the parts of the sentence | `as`, `with`, `to`, `from`, `in`, `of`, `on`, `at`, `where`, `is`, `if`, `otherwise`, `and` |
| **Silent word** | Makes it read naturally and is skipped | `the`, `a`, `an`, `that`, `it` |

Every new sentence below follows this shape, and each one comes with its word breakdown. Silent words are optional: `stop the program` and `stop program` are the same.

Three more rules:

- Use what Enlng already has. Nothing new is added for a job Enlng can already do.
- Add no new symbols.
- Give each word one meaning.

---

## Part 1. Already in Enlng (nothing new)

Each of these was run with the current build.

| Need | Write this today |
|---|---|
| Is a value in a list | `when the numbers contains 3:` |
| Power | `pow(2, 10)` gives `1024` |
| Fixed named values | `freeze RED as "red"` |
| Values without repeats | `unique(numbers)` |
| Text with values inside | `f"{name} has {score} points"` |
| A check that must hold | `check(total is 10, "total is wrong")` |
| Many cases | `when ... otherwise when ... otherwise` |
| Comments | `#` at the start of each line |
| Many values into a function | pass a list: `total([1, 2, 3])` |

---

## Part 2. New sentences

### 1. Choosing by a value

```
when the color is:
    "red": show "stop"
    "green" or "blue": show "go"
    otherwise: show "wait"
```

| Action | Connectors | Silent |
|---|---|---|
| `when` | `is`, `or`, `otherwise` | `the` |

It reads as: "When the color is red, show stop."

### 2. Picking one of two values

```
remember the label as "adult" if the age is at least 18 otherwise "child"
```

| Action | Connectors | Silent |
|---|---|---|
| `remember` | `as`, `if`, `is at least`, `otherwise` | `the` |

### 3. Is it in the list

```
when 3 is in the numbers:
when "x" is not in the word:
```

| Action | Connectors | Silent |
|---|---|---|
| `when` | `is in`, `is not in` | `the` |

### 4. Power

```
show 2 to the power of 10
show 5 squared
```

| Action | Connectors | Silent |
|---|---|---|
| `show` | `to ... power of`, `squared` | `the` |

### 5. Taking a list or map apart

```
remember the name and the age from the person
for each name and score in the results:
```

| Action | Connectors | Silent |
|---|---|---|
| `remember`, `for each` | `and`, `from`, `in` | `the` |

### 6. Making a list from a list

```
remember big as every n in the numbers where n is greater than 10
remember doubled as n times 2 for every n in the numbers
```

| Action | Connectors | Silent |
|---|---|---|
| `remember` | `as`, `every ... in`, `where`, `for every` | `the` |

### 7. Giving inputs by name

```
show area with width as 3 and height as 4
```

| Action | Connectors | Silent |
|---|---|---|
| `show` | `with`, `as`, `and` | (none) |

### 8. Making sure a value is the right kind

```
function area with width, height:
    check that width is a number
    check that height is a number
    give width times height
```

| Action | Connectors | Silent |
|---|---|---|
| `check` | `is` | `that`, `a` |

Kinds: `number`, `whole number`, `text`, `list`, `map`, `function`, or a class name.

When the kind can be known while the file is read, the mistake is reported before the program runs.

### 9. Ending the program

```
stop the program
```

| Action | Connectors | Silent |
|---|---|---|
| `stop` | (none) | `the` |

### 10. Tests

```
test "adding works":
    check that add(1, 2) is 3
    check that add(-1, 1) is 0
```

| Action | Connectors | Silent |
|---|---|---|
| `test`, `check` | `is` | `that` |

Run `enlng test`.

### 11. Doing things at the same time

```
run together:
    remember a as http_get("https://a.com")
    remember b as http_get("https://b.com")
show a
show b
```

| Action | Connectors | Silent |
|---|---|---|
| `run` | `together` | (none) |

The lines inside run at the same time. The program goes on when all of them are done.

### 12. Files that are not text (images, ...)

```
remember the data as read bytes from "photo.png"
write the data to "copy.png"
```

| Action | Connectors | Silent |
|---|---|---|
| `read`, `write` | `from`, `to` | `the` |

### 13. A small web server

```
function answer with request:
    give "Hello from Enlng"

serve answer on port 8080
```

| Action | Connectors | Silent |
|---|---|---|
| `serve` | `on` | (none) |

### 14. Network connections

```
remember the server as listen on port 8080
remember the client as accept from the server
remember the line as receive from the client
send "hello" to the client
close the client

remember the link as connect to "example.com" at port 80
```

| Action | Connectors | Silent |
|---|---|---|
| `listen`, `accept`, `receive`, `send`, `close`, `connect` | `on`, `from`, `to`, `at` | `the` |

### 15. Big whole numbers

There is no new sentence. A whole number too big for 64 bits stays exact on its own:

```
show 2 to the power of 100      # 1267650600228229401496703205376
```

---

## Part 3. Tools (no syntax)

- **Interactive mode:** type `enlng` with no file, then write lines one by one.
- **Tests:** `enlng test` runs every file ending in `_test.enlng`.
- **Editor help:** errors underlined while typing, completion, hover help and go to definition, in VS Code and Enlangg Studio.
- **Debugger:** click a line in the editor to stop there and see the values.
- **Packages:** `enlngm add charts`, then `use "charts"`.
- **Release:** a new version, so the installed `enlng` (4.0.0 today) is replaced.

---

## Left out on purpose

| Idea from other languages | Why |
|---|---|
| `match` | `when the color is:` does it |
| `**`, `^`, `/* */` | Words do the same job |
| `enum` | `freeze` does it |
| A separate `set` type | `unique` and `contains` cover everyday use |
| `format(...)` | f-strings do it |
| Any number of inputs (`...xs`) | Pass a list |
| `yield` and generators | Rarely needed, and hard to explain to a beginner |
| Channels and locks | `run together:` covers everyday use |
