# References & Dereferencing

Lumen variables can be referenced and dereferenced directly, independent of the `&var` syntax used to pass output parameters to built-ins.

```lumen
value = 20

ref = &value

deref = *ref
```

| Operator | Meaning |
|---|---|
| `&value` | Take a reference to a variable's storage slot |
| `*ref` | Dereference — read the value a reference points to |

A reference can be stored in a variable like any other value and passed around, then dereferenced later with `*` to read the current contents of the variable it points to.

> Currently, dereferenced assignments (e.g. `*ref = 10`) are not supported.