# 👑 Enlang Sovereign Pointer & Memory Reference Guide

> **Sovereign Specification**: Enlang AG 2.0  
> **Standard Library**: `lib_std.enlng`  
> **Philosophy**: Zero Cryptic Symbols (`*`, `&`), Zero Segmentation Faults, 100% Pure Natural English Connected Grammar.

---

## 1. What is a Pointer in Enlang?

In traditional systems languages like C, pointers require confusing symbols (`*`, `&`, `->`) and dangerous memory math that frequently crashes programs with **Segmentation Faults**.

In **Sovereign Enlang**, a pointer is simply a **Memory Reference Entity** that holds a value inside a shared memory box. 

* **Normal Scalar Variable (`remember x as 500`)**: Stores a direct number. Passing it to a function creates a local copy (call-by-value). Modifying it inside a function does NOT alter the caller's variable.
* **Pointer Variable (`remember a as pointer with 500`)**: Creates an entity on the memory heap. Passing it to a function shares the exact same memory box (call-by-reference). Modifying it inside a function directly updates the caller's variable in place!

---

## 2. Core English Grammar for Pointers

Enlang provides a continuous, fully connected English vocabulary for pointer operations:

| Operation | Pure Enlang Syntax | What It Does |
|---|---|---|
| **Create Pointer** | `remember a as pointer with 500` | Allocates a memory box holding initial value `500`. |
| **Dereference (Read)** | `value of a` | Reads the current value inside the memory box. |
| **In-Place Mutation** | `change value of a to 350` | Overwrites the value directly in the shared memory box. |
| **Pointer Swapping** | `swap_pointers with a, b` | Swaps the values between two pointers in place. |
| **Library Getter** | `get_pointer with a` | Functional helper to read a pointer. |
| **Library Setter** | `set_pointer with a, 350` | Functional helper to update a pointer. |

---

## 3. Real-World Example: Multi-Account Bank Transfer

In this example, two account pointers (`alice` and `bob`) are passed into a function. The function updates both balances directly in place:

```enlng
type enlng
use "lib_std.enlng"

# Function that receives two account pointers and an amount
function transfer with from_account, to_account, amount:
    remember sender_bal as value of from_account
    remember receiver_bal as value of to_account
    
    change value of from_account to (sender_bal minus amount)
    change value of to_account to (receiver_bal plus amount)

# Create two account pointers:
remember alice as pointer with 500
remember bob as pointer with 200

# Execute transfer in place:
transfer with alice, bob, 150

show value of alice    # 350
show value of bob      # 350
```

---

## 4. Why Primitive Numbers Cannot Mutate In Place

When you write:
```enlng
function deduct with balance, amount:
    balance = balance minus amount

remember account as 500
deduct with account, 150
show account    # Still 500!
```

### Why it prints `500`:
1. `500` is an immutable primitive scalar (`ENLNG_VAL_INT`).
2. The runtime passes numbers by copying their value.
3. `balance` inside `deduct` is an isolated local copy. When `deduct` ends, that copy vanishes.
4. Wrapping the value in a pointer box (`pointer with 500` or `{"value": 500}`) allocates heap memory, allowing callers and callees to share the exact same storage.

---

## 5. Standard Student Library (`lib_std.enlng`) Reference

To enable student productivity and zero-symbol coding, import `lib_std.enlng`:

```enlng
use "lib_std.enlng"
```

Available Primitives:
* **`pointer with val`**: Creates a named reference entity `{"value": val}`.
* **`get_pointer with p`**: Retrieves the dereferenced value.
* **`set_pointer with p, val`**: Sets the dereferenced value in place.
* **`swap_pointers with a, b`**: Swaps the contents of two pointers.
* **`make_cell with val`**: Creates an indexed container `[val]`.
* **`get_cell with c`**: Retrieves `c at 0`.
* **`set_cell with c, val`**: Sets `c at 0 = val`.
* **`swap_cells with a, b`**: Swaps contents of two container cells.
* **`read_number`**: Reads an integer from standard input.
* **`read_numbers_line`**: Reads a space-separated line of numbers into a list.
* **`abs_val with n`**: Returns absolute value.
* **`difference with a, b`**: Returns `|a - b|`.
* **`max_of with a, b` / `min_of with a, b`**: Boundary helpers.
* **`sum_of_list with items`**: Returns the sum of all elements.
* **`sort_list with items`**: In-place spatial pair sorting.
