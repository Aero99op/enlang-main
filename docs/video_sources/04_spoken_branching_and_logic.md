# The Complete Enlangg Masterclass: Zero to Native Production
## Module 04: Decision Logic — Spoken Branching, Relational Conditions & Fallbacks

---

### Lecture Overview & Learning Objectives
Welcome back to Module 4 of the Enlangg Masterclass! In the previous module, we conquered Spoken Math and numeric computing. Now, we are giving our programs the power to think, make intelligent decisions, and enforce business rules.

In traditional programming languages, branching logic is cluttered with confusing symbols like `&&`, `||`, `!`, `===`, and nested curly braces `{ }`. A single misplaced character can invert security rules or cause silent bugs.

Enlangg replaces symbolic operators with **Spoken Relational Clauses**.

By the end of this lecture, you will master:
1. Writing clean conditional branches with `if`, `when`, `else if`, `otherwise if`, `else`, and `otherwise`.
2. The complete suite of spoken relational operators: `is greater than`, `is less than`, `is equal to`, `is not equal to`, `is at least`, and `is at most`.
3. Compound conditions with natural boolean connectors: `and`, `or`, and `not`.
4. The powerful `contains` keyword for checking collection membership.
5. Building a production-grade **Security Authentication & Role-Based Access Control (RBAC) Gateway**.
6. Completing a hands-on coding challenge with full line-by-line code displayed on screen.

Let’s open our code editor and inspect the full code!

---

### Part 1: Hands-On Code Display — The Basic If / Else Branch

Let's start with our first complete, runnable script. In Enlangg, conditional checks read like plain English sentences ending with a colon `:`, followed by clean 4-space indentation.

#### Full Source Code Display: `04_basic_branching.enlng`
```enlng
type enlng

// Variable initialization
set user_age to 19

// Decision branch with spoken comparison
if user_age is greater than or equal to 18:
    display "Eligibility Status: Eligible to Vote"
    display "Access Granted to Democratic Portal"
else:
    display "Eligibility Status: Ineligible (Underage)"
    display "Access Denied: Please re-apply at age 18"
```

#### Terminal Execution Output:
```text
Eligibility Status: Eligible to Vote
Access Granted to Democratic Portal
```

#### Code Breakdown:
- Notice `is greater than or equal to`: there are no cryptic `>=` symbols required.
- The block of code inside the branch is indented by 4 spaces.
- No parentheses around the condition, and zero curly braces `{ }`.

---

### Part 2: Relational Comparison Operators Reference Guide

Enlangg provides a 1-to-1 spoken English equivalent for every classical mathematical relational operator:

| Classical Symbol | Spoken Enlangg Clause | Example In Code |
| :--- | :--- | :--- |
| `==` | `is equal to` or `equals` | `if status is equal to "active":` |
| `!=` | `is not equal to` | `if response is not equal to 404:` |
| `>` | `is greater than` | `if score is greater than 100:` |
| `<` | `is less than` | `if temperature is less than 0:` |
| `>=` | `is greater than or equal to` or `is at least` | `if age is at least 21:` |
| `<=` | `is less than or equal to` or `is at most` | `if retries is at most 3:` |

#### Full Source Code Display: `04_relational_operators.enlng`
```enlng
type enlng

set account_tier to "VIP"
set account_balance to 15000
set failed_attempts to 1

// Evaluating equality, threshold, and minimums
if account_tier is equal to "VIP":
    display "Priority Lane Activated"

if account_balance is at least 10000:
    display "High-Net-Worth Liquidity Verified"

if failed_attempts is at most 3:
    display "Account Security State: Normal"
```

#### Terminal Execution Output:
```text
Priority Lane Activated
High-Net-Worth Liquidity Verified
Account Security State: Normal
```

---

### Part 3: Multi-Tier Decision Chains (`else if` & `otherwise if`)

When handling complex multiple-choice logic, Enlangg supports both `else if` and the natural synonym `otherwise if`, as well as `else` or `otherwise`:

#### Full Source Code Display: `04_grade_evaluator.enlng`
```enlng
type enlng

set student_score to 87

if student_score is greater than or equal to 90:
    display "Academic Honors: Grade A (Distinction)"
otherwise if student_score is at least 80:
    display "Academic Honors: Grade B (Commended)"
otherwise if student_score is at least 70:
    display "Academic Honors: Grade C (Satisfactory)"
otherwise if student_score is at least 60:
    display "Academic Honors: Grade D (Pass)"
otherwise:
    display "Academic Warning: Grade F (Remedial Action Required)"
```

#### Terminal Execution Output:
```text
Academic Honors: Grade B (Commended)
```

#### Why This Prevents Semantic Errors:
In legacy languages, mixing up `=` and `==` in an `if` statement silently reassigns the variable, leading to devastating security bugs. In Enlangg, assignment is always `set <var> to <val>`, and equality checking is `is equal to`. The two operations can never be accidentally conflated!

---

### Part 4: Compound Logic — `and`, `or`, `not`

Real applications make decisions based on multiple intersecting factors. Enlangg uses the plain English words `and`, `or`, and `not`.

#### Full Source Code Display: `04_compound_security.enlng`
```enlng
type enlng

set is_authenticated to true
set user_security_clearance to 5
set is_ip_blacklisted to false

// Combining multiple conditions with spoken boolean logic
if is_authenticated is equal to true and user_security_clearance is at least 4 and not is_ip_blacklisted:
    display "SECURE TERMINAL: Access Granted to Classified Mainframe"
else:
    display "SECURITY ALERT: Access Denied to Mainframe"
```

#### Terminal Execution Output:
```text
SECURE TERMINAL: Access Granted to Classified Mainframe
```

#### The `contains` Keyword for Collection Membership:
You can also verify whether a list or string contains a specific element:

```enlng
type enlng

set allowed_roles to ["admin", "super_admin", "auditor"]
set current_user_role to "super_admin"

if allowed_roles contains current_user_role:
    display "Role Verification Succeeded: Privileged Operations Permitted"
else:
    display "Role Verification Failed: Insufficient Permissions"
```

---

### Part 5: Complete Production System — Financial Fraud Detection Engine

Let’s study a complete, enterprise-grade fraud detection script. Notice how the entire business rule matrix reads like a regulatory compliance specification document:

#### Full Source Code Display: `04_fraud_detection_gateway.enlng`
```enlng
type enlng

// Transaction Profile
set transaction_id to "TX-99402"
set transaction_amount to 8500.00
set destination_country to "Switzerland"
set is_card_present to false
set risk_score to 15

// Fraud Rule 1: High Dollar Unverified Velocity
if transaction_amount is greater than 10000.00 and not is_card_present:
    increase risk_score by 50
    display "Flag Triggered: Large Unverified Remote Transaction"

// Fraud Rule 2: Moderate Value Offshore Transfer
otherwise if transaction_amount is at least 5000.00:
    increase risk_score by 20
    display "Flag Triggered: Elevated Value Offshore Transfer"

// Fraud Rule 3: Destination Cross-Check
if destination_country is equal to "Switzerland" or destination_country is equal to "Cayman Islands":
    increase risk_score by 15
    display "Flag Triggered: Monitored Financial Jurisdiction"

// Section 2: Final Adjudication
display "=========================================="
display "       TRANSACTION RISK ASSESSMENT        "
display "=========================================="
display "Transaction ID:       ", transaction_id
display "Calculated Risk Score:", risk_score

if risk_score is greater than or equal to 50:
    display "DECISION: TRANSACTION REJECTED (HARD DECLINE)"
otherwise if risk_score is at least 30:
    display "DECISION: TRANSACTION PENDING (MANUAL REVIEW REQUIRED)"
otherwise:
    display "DECISION: TRANSACTION APPROVED (PROCEED TO SETTLEMENT)"
display "=========================================="
```

#### Terminal Execution Output:
```text
Flag Triggered: Elevated Value Offshore Transfer
Flag Triggered: Monitored Financial Jurisdiction
==========================================
       TRANSACTION RISK ASSESSMENT        
==========================================
Transaction ID:        TX-99402
Calculated Risk Score: 50
DECISION: TRANSACTION REJECTED (HARD DECLINE)
==========================================
```

---

### Part 6: Best Practices for Writing Clean Conditional Logic

1. **Avoid Deeply Nested If-Statements**:
   Prefer flat guard clauses with `else if` / `otherwise if` rather than nesting 5 levels of `if` blocks inside each other.
2. **Use Descriptive Boolean Variables**:
   Instead of writing `if user_age >= 65 and retired == true:`, declare a clear variable: `set is_senior_citizen to true` and then write `if is_senior_citizen:`.
3. **Always Include a Default `else` / `otherwise` Fallback**:
   In mission-critical infrastructure, an unhandled edge case can cause silent failure. Always provide an `else:` branch that logs unexpected states.

---

### Part 7: Hands-On Challenge — The Server Health & Auto-Scale Sentinel

Now it's your turn to write the code!

#### The Challenge:
You are building an automatic load balancer sentinel that inspects cluster health and decides whether to scale up servers, alert engineers, or maintain normal operations:
1. Declare `cluster_cpu_load` as `88.5` (percent).
2. Declare `active_connections` as `4500`.
3. Declare `is_peak_hours` as `true`.
4. If `cluster_cpu_load` is greater than `85.0` and `active_connections` is at least `4000`:
   - Display `"CRITICAL LOAD: Spawning 4 New Container Nodes Immediately"`.
5. Otherwise if `cluster_cpu_load` is at least `70.0` or `is_peak_hours` is equal to `true`:
   - Display `"MODERATE LOAD: Preparing Standby Nodes in Warm Pool"`.
6. Otherwise:
   - Display `"NORMAL LOAD: Cluster Health Optimal"`.
7. Print a formatted telemetry summary report.

#### Pause your screen and code this out!

---

### Solution Walkthrough: Full Code Display

#### Full Source Code Display: `cluster_sentinel.enlng`
```enlng
type enlng

// Step 1: Telemetry Data
set cluster_id to "US-EAST-K8S-04"
set cluster_cpu_load to 88.5
set active_connections to 4500
set is_peak_hours to true

// Step 2: Formatted Telemetry Header
display "=========================================="
display "       CLUSTER HEALTH MONITOR V4          "
display "=========================================="
display "Cluster Node:        ", cluster_id
display "CPU Load:            ", cluster_cpu_load, "%"
display "Active Connections:  ", active_connections
display "Peak Hours Active:   ", is_peak_hours
display "------------------------------------------"

// Step 3: Autoscale Decision Matrix
if cluster_cpu_load is greater than 85.0 and active_connections is at least 4000:
    display "ACTION: CRITICAL LOAD -> Spawning 4 New Container Nodes Immediately"
otherwise if cluster_cpu_load is at least 70.0 or is_peak_hours is equal to true:
    display "ACTION: MODERATE LOAD -> Preparing Standby Nodes in Warm Pool"
otherwise:
    display "ACTION: NORMAL LOAD -> Cluster Health Optimal"

display "=========================================="
```

#### Expected Terminal Output:
```text
==========================================
       CLUSTER HEALTH MONITOR V4          
==========================================
Cluster Node:        US-EAST-K8S-04
CPU Load:            88.5 %
Active Connections:  4500
Peak Hours Active:   true
------------------------------------------
ACTION: CRITICAL LOAD -> Spawning 4 New Container Nodes Immediately
==========================================
```

---

### Lecture Summary & Next Steps
Congratulations on finishing Module 4!

In this lecture, you mastered:
- Spoken conditional branching with `if`, `else if`, `otherwise if`, and `else`.
- Natural relational operators like `is greater than`, `is at least`, `is equal to`, and `is not equal to`.
- Combining complex logic with `and`, `or`, and `not`.
- The `contains` operator for fast collection lookup.
- Designing industrial-grade fraud detection and auto-scaling decision engines.

In the next lecture, **Module 05: Loops & Bounded Iteration**, we will learn how to repeat work without writing repetitive code using `while`, `until`, `for each`, and `for range`.

Keep coding, and see you in Module 5!
