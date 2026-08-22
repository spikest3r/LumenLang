# Age Calculator

Generate it:

```bash
lumen --examples age
```

```lumen
yearNow = 2026
userYear = 0
println 'Hello, world!'
print 'Enter your birth year: '
inputInt &userYear
age = yearNow - userYear
print 'Your age: '
println age
```

A minimal but complete program: it declares two variables, reads an integer from the user with `inputInt`, does one subtraction, and prints the result with a mix of `print` and `println`.

Touches: [variables](../language-guide/variables-and-values.md), [input/output](../language-guide/input-output.md), [arithmetic operators](../language-guide/operators.md).

## Scan into Android

![QR for the age example](../images/qrcodes/age.png)

See [Lumen on Android](../lumen-in-apps/lumen-on-android.md) for how the scan-and-run pipeline works.
