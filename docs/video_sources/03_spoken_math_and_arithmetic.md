# The Complete Enlangg Masterclass: Zero to Native Production
## Module 03: Spoken Math, Arithmetic Operations & Numeric Computing

---

### Lecture Overview & Learning Objectives
Welcome back to Module 3 of the Enlangg Masterclass! In the previous lecture, we mastered deterministic memory slots and saw how Enlangg gives us memory safety and variable freedom with zero garbage collector lag.

In this lecture, we take the next major leap: **Spoken Math and High-Speed Numeric Computation**.

By the end of this lecture, you will:
1. Master all natural spoken arithmetic operators: `plus`, `minus`, `multiplied by`, and `divided by`.
2. Understand remainder and cyclical math with `modulo` / `mod`.
3. Learn how operator precedence and parenthetical grouping work in Enlangg.
4. Utilize built-in mathematical constants and functions (`sqrt`, `floor`, `ceil`, `pi`).
5. Write a real-world financial and e-commerce billing engine using pure spoken English equations.
6. Complete a hands-on coding challenge to solidify your computational skills.

Let’s get coding!

---

### Part 1: The Mental Model — Math Without Symbol Fatigue

In traditional languages, math is dominated by ASCII symbols:
- `+`, `-`, `*`, `/`, `%`

While simple for single operations, real-world engineering code quickly becomes dense, unreadable ASCII soup—especially when mixed with pointers, dereferencing, and typecasts. For example, in C or Go, an asterisk `*` can mean multiplication, a pointer type declaration, or a pointer dereference! This symbol overloading creates cognitive friction and dangerous bugs.

Enlangg introduces **Spoken Arithmetic**:
- Mathematical operations are written as fluent English clauses: `plus`, `minus`, `multiplied by`, and `divided by`.
- Under the hood, the Enlangg compiler translates these directly into single-cycle CPU hardware instructions: `ADD`, `SUB`, `MUL`, and `DIV`.
- You get code that reads like an executive financial report or scientific paper, executing at native silicon speeds.

---

### Part 2: The Core Spoken Arithmetic Operators

Let's open our sandbox and examine the primary arithmetic operations in action:

#### 1. Addition with `plus`
```enlng
type enlng

set base_salary to 75000
set annual_bonus to 15000
set total_compensation to base_salary plus annual_bonus

display "Total Package: $", total_compensation
```
The token `plus` translates directly to native addition (`+`).

#### 2. Subtraction with `minus`
```enlng
type enlng

set account_balance to 1000
set withdrawal_amount to 250
set remaining_balance to account_balance minus withdrawal_amount

display "Remaining Balance: $", remaining_balance
```
The token `minus` translates directly to native subtraction (`-`).

#### 3. Multiplication with `multiplied by`
```enlng
type enlng

set hourly_rate to 65
set hours_worked to 40
set gross_earnings to hourly_rate multiplied by hours_worked

display "Gross Earnings: $", gross_earnings
```
Notice how natural `multiplied by` reads. It leaves zero ambiguity about whether an asterisk is a pointer or a multiplication.

#### 4. Division with `divided by`
```enlng
type enlng

set total_bill to 360
set guest_count to 4
set per_person_share to total_bill divided by guest_count

display "Each Person Pays: $", per_person_share
```
`divided by` translates directly to native floating-point or integer division.

#### 5. Modulo & Remainder with `modulo` or `mod`
```enlng
type enlng

set total_packets to 105
set batch_capacity to 10
set leftover_packets to total_packets modulo batch_capacity

display "Unpacked Leftovers: ", leftover_packets
```
Enlangg accepts `modulo`, `mod`, or `modulus` interchangeably.

---

### Part 3: Precedence and Parentheses Grouping

Just like in classical mathematics, operations in Enlangg follow standard algebraic precedence (multiplication and division take precedence over addition and subtraction).

However, when building complex formulas, best practice is to make your intent crystal clear using parentheses `()`:

```enlng
type enlng

set exam_score1 to 88
set exam_score2 to 94
set exam_score3 to 91

// Explicit parenthetical grouping ensures addition completes before division
set average_score to (exam_score1 plus exam_score2 plus exam_score3) divided by 3

display "Semester Grade Average: ", average_score
```

#### Why Parentheses Matter:
Without parentheses:
```enlng
set result to exam_score1 plus exam_score2 plus exam_score3 divided by 3
```
In this case, only `exam_score3` would be divided by 3. Grouping with `( ... )` documents your architectural intent and eliminates calculation bugs.

---

### Part 4: Built-in Scientific & Math Utilities

Enlangg includes native mathematical constants and functions directly in the runtime—no header imports or external packages required:

```enlng
type enlng

// Universal Constants
display "Value of Pi: ", pi
display "Euler's Number: ", e

// Square Root
set hypotenuse_squared to 25
set side_length to sqrt(hypotenuse_squared)
display "Side Length: ", side_length

// Floor and Ceiling
set raw_rate to 99.45
set floor_rate to floor(raw_rate)
set ceil_rate to ceil(raw_rate)

display "Floor: ", floor_rate
display "Ceil:  ", ceil_rate

// Pseudo-Random Number Generation
set dice_roll to random_number(1, 6)
display "Dice Roll: ", dice_roll
```

---

### Part 5: Complete Real-World System — Enterprise Payroll & Tax Calculator

Let’s bring everything together into a production-grade script that calculates a detailed salary breakdown including overtime, bonus multipliers, tax withholding, and health insurance deductions:

```enlng
type enlng

// Employee Base Data
set employee_name to "Dr. Elena Rostova"
set regular_hours to 40
set overtime_hours to 8
set hourly_pay_rate to 50.00
set overtime_multiplier to 1.5

// Section 1: Earnings Computation
set base_pay to regular_hours multiplied by hourly_pay_rate
set overtime_pay_rate to hourly_pay_rate multiplied by overtime_multiplier
set overtime_earnings to overtime_hours multiplied by overtime_pay_rate
set gross_income to base_pay plus overtime_earnings

// Section 2: Deductions & Taxes
set federal_tax_rate to 0.20
set state_tax_rate to 0.05
set health_insurance_deduction to 150.00

set federal_tax to gross_income multiplied by federal_tax_rate
set state_tax to gross_income multiplied by state_tax_rate
set total_deductions to federal_tax plus state_tax plus health_insurance_deduction

// Section 3: Final Net Calculation
set net_take_home to gross_income minus total_deductions

// Section 4: Formatted Paystub Output
display "=========================================="
display "       CORPORATE PAYROLL STATEMENT        "
display "=========================================="
display "Employee:              ", employee_name
display "Base Salary:          $", base_pay
display "Overtime Earnings:    $", overtime_earnings
display "Gross Total Income:   $", gross_income
display "------------------------------------------"
display "Federal Tax (20%):    $", federal_tax
display "State Tax (5%):       $", state_tax
display "Health Insurance:     $", health_insurance_deduction
display "Total Deductions:     $", total_deductions
display "=========================================="
display "NET TAKE-HOME PAY:    $", net_take_home
display "=========================================="
```

#### Output:
```text
==========================================
       CORPORATE PAYROLL STATEMENT        
==========================================
Employee:              Dr. Elena Rostova
Base Salary:          $ 2000.0
Overtime Earnings:    $ 600.0
Gross Total Income:   $ 2600.0
------------------------------------------
Federal Tax (20%):    $ 520.0
State Tax (5%):       $ 130.0
Health Insurance:     $ 150.0
Total Deductions:     $ 800.0
==========================================
NET TAKE-HOME PAY:    $ 1800.0
==========================================
```

---

### Part 6: Best Practices & Pro-Tips for Numeric Computing

1. **Keep Equations Readable**: Break down long multi-clause math into logical intermediate variables (e.g. calculate `gross_income` first, then `deductions`, then `net_pay`).
2. **Combine Spoken Operators with Natural Mutations**:
   You can calculate an initial value with spoken operators, and then update it over time using spoken mutations:
   ```enlng
   set cart_total to item_price multiplied by quantity
   decrease cart_total by discount_coupon
   ```
3. **Floating Point Precision**: When computing currency, remember that all floating-point numbers in modern CPUs use IEEE-754 standards. Always round or floor final displays when presenting customer-facing receipts.

---

### Part 7: Hands-On Challenge — The Rocket Physics Trajectory Simulator

Now it's your turn to write the code!

#### The Challenge:
A lunar exploration craft is calculating its fuel burn and descent velocity.
1. Declare initial descent velocity: `150.0` meters/second.
2. Declare gravity acceleration: `1.62` meters/second squared.
3. Declare burn duration: `10` seconds.
4. Calculate additional speed gained from gravity: `gravity_acceleration multiplied by burn_duration`.
5. Declare retro-thruster deceleration counter-force: `35.0` meters/second.
6. Calculate final touchdown velocity: `initial_velocity plus gravity_gain minus retro_force`.
7. Print a formatted telemetry readout of all computed metrics.

#### Pause your screen and try it out!

---

### Solution Walkthrough: `rocket_telemetry.enlng`

```enlng
type enlng

set mission_id to "Artemis-Payload-X"
set initial_velocity to 150.0
set lunar_gravity to 1.62
set burn_seconds to 10
set thruster_counter_force to 35.0

// Step 1: Calculate gravity speed addition
set gravity_speed_increase to lunar_gravity multiplied by burn_seconds

// Step 2: Compute final velocity
set final_velocity to (initial_velocity plus gravity_speed_increase) minus thruster_counter_force

// Step 3: Telemetry Printout
display "=========================================="
display "       LUNAR DESCENT TRAJECTORY           "
display "=========================================="
display "Mission:             ", mission_id
display "Initial Speed:       ", initial_velocity, " m/s"
display "Gravity Addition:    ", gravity_speed_increase, " m/s"
display "Thruster Braking:   -", thruster_counter_force, " m/s"
display "------------------------------------------"
display "Touchdown Velocity:  ", final_velocity, " m/s"
display "=========================================="
```

#### Expected Terminal Output:
```text
==========================================
       LUNAR DESCENT TRAJECTORY           
==========================================
Mission:             Artemis-Payload-X
Initial Speed:       150.0 m/s
Gravity Addition:    16.2 m/s
Thruster Braking:   - 35.0 m/s
------------------------------------------
Touchdown Velocity:  131.2 m/s
==========================================
```

---

### Lecture Summary & Next Steps
Congratulations on finishing Module 3!

In this lecture, you learned:
- How Enlangg replaces cryptic ASCII math symbols with conversational clauses: `plus`, `minus`, `multiplied by`, and `divided by`.
- How remainder calculations work with `modulo`.
- Using parentheses `()` to enforce formula order of operations.
- Native mathematical functions like `sqrt()`, `floor()`, and `ceil()`.
- Building real-world financial payroll and physics simulation scripts.

In the next lecture, **Module 04: Decision Logic & Branching**, we will look at conditional decisions: `if`, `otherwise if`, `otherwise`, and relational operators (`is greater than`, `is equal to`) to give our programs the power to think and make intelligent decisions!

Keep practicing, and see you in the next lecture!
