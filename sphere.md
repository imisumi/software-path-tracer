# Ray-Sphere Intersection

A comprehensive guide to the mathematical foundation of ray-sphere intersection testing in ray tracing applications.

## How to Read Mathematical Notation (For Beginners)

Before diving into the math, let's learn how to read and understand mathematical notation step by step.

### Reading Symbol by Symbol

Mathematical notation is just a shorthand way of writing instructions. Think of it like reading a recipe - each symbol tells you what to do.

### Example 1: Reading |P - C|² = r²

Let's break this down piece by piece:

**Step 1: Identify the components**
```
|P - C|² = r²
 │ │ │ │   │
 │ │ │ │   └── r squared (radius × radius)
 │ │ │ └── squared (multiply by itself)
 │ │ └── minus (subtraction)
 │ └── P (a point in 3D space)
 └── "length of" or "distance of"
```

**Step 2: Read from inside out**
1. `P - C` = "Take point P, subtract point C"
2. `|P - C|` = "Find the distance of (P - C)"
3. `|P - C|²` = "Square that distance"
4. `r²` = "Square the radius"
5. `=` = "equals"

**Step 3: Put it together**
"The squared distance from point P to point C equals the squared radius"

**Step 4: Convert to code**
```cpp
glm::vec3 P;  // Some point
glm::vec3 C;  // Center point
float r;      // Radius

// |P - C|² = r² becomes:
glm::vec3 diff = P - C;                    // P - C
float distance = glm::length(diff);       // |P - C|
float distanceSquared = distance * distance;  // |P - C|²
float radiusSquared = r * r;               // r²
bool onSphere = (distanceSquared == radiusSquared);  // =
```

### Example 2: Reading R(t) = O + tD

**Step 1: Identify the components**

Breaking down `R(t) = O + tD`:

- `R` = result (the point we're calculating)
- `(t)` = function notation (means "depends on t")
- `=` = equals
- `O` = origin (starting point)
- `+` = plus (addition)
- `t` = parameter (how far to go)
- `D` = direction (which way to go)

**Step 2: Read it**
"R, which depends on t, equals O plus t times D"

**Step 3: What it means**
"The result point R depends on parameter t. Start at origin O, then move t distance in direction D"

**Step 4: Convert to code**
```cpp
glm::vec3 O;  // Origin (starting point)
glm::vec3 D;  // Direction (which way to go)
float t;      // How far to go

// R(t) = O + tD becomes:
glm::vec3 R = O + t * D;  // Final point
```

## Common Mathematical Notation Patterns

### Variables and Points
- **Single letters (P, C, O, D, r)** = Variables (numbers or points)
- **Lowercase (t, x, y, z)** = Usually scalars (single numbers)
- **Uppercase (P, C, O, D)** = Usually vectors or points

### Operations
- **+, -, ×** = Add, subtract, multiply (same as regular math)
- **·** = Dot product (special vector multiplication)
- **| |** = Length, distance, or absolute value
- **²** = Squared (multiply by itself)
- **√** = Square root

### Functions
- **f(x)** = "Function f that depends on x"
- **R(t)** = "Point R that depends on parameter t"

### Reading Strategy
1. **Start from the inside** (parentheses first)
2. **Work outward** (like peeling an onion)
3. **Left to right** for same-level operations
4. **Think in terms of "what am I calculating?"**

## Converting Notation to Code - Step by Step

### Pattern 1: Distance/Length
**Math:** `|A - B|`
**Meaning:** Distance from A to B
**Code:**
```cpp
float distance = glm::length(A - B);
```

### Pattern 2: Squared Distance (Faster)
**Math:** `|A - B|²`
**Meaning:** Distance squared (avoids expensive square root)
**Code:**
```cpp
glm::vec3 diff = A - B;
float distanceSquared = glm::dot(diff, diff);
```

### Pattern 3: Parametric Equations
**Math:** `P(t) = A + tB`
**Meaning:** Point P moves along line from A in direction B
**Code:**
```cpp
glm::vec3 P = A + t * B;
```

### Pattern 4: Dot Product
**Math:** `A · B`
**Meaning:** How much A and B point in same direction
**Code:**
```cpp
float result = glm::dot(A, B);
```

## Practice Examples

Try converting these to code:

**1. Math:** `|V|` 
**Your code:** `________________`
**Answer:** `glm::length(V)`

**2. Math:** `A · A`
**Your code:** `________________`
**Answer:** `glm::dot(A, A)`

**3. Math:** `P(s) = Q + sR`
**Your code:** `________________`
**Answer:** `glm::vec3 P = Q + s * R;`

## Overview

Ray-sphere intersection is one of the fundamental operations in ray tracing, used to determine where a ray intersects with spherical objects in 3D space. This document provides the complete mathematical derivation and implementation details.

## Mathematical Foundation

### Ray Representation

A ray is defined parametrically as:
```
R(t) = O + tD
```
Where:
- `O` = Ray origin (3D point)
- `D` = Ray direction (normalized 3D vector)
- `t` = Parameter (distance along ray, t ≥ 0)

### Sphere Representation

A sphere is defined implicitly as:
```
|P - C|² = r²
```
Where:
- `C` = Sphere center (3D point)
- `r` = Sphere radius (scalar)
- `P` = Any point on the sphere surface

## Intersection Equation Derivation

Let's walk through this step-by-step with clear explanations:

### Step 1: What are we trying to find?
We want to know: "Where does our ray hit the sphere?" 

The ray equation `R(t) = O + tD` gives us any point along the ray.
The sphere equation `|P - C|² = r²` tells us if a point P is on the sphere.

So we ask: "For what value of `t` does the ray point `O + tD` lie on the sphere?"

### Step 2: Substitute Ray into Sphere Equation
Replace `P` in the sphere equation with our ray equation:
```
|O + tD - C|² = r²
```

**In plain English:** "The distance from (ray point) to (sphere center), squared, equals radius squared"

### Step 3: Simplify with Vector Substitution
Let's make this easier to work with. Let `L = O - C` (vector from sphere center to ray origin):
```
|L + tD|² = r²
```

**What L represents:**
```cpp
glm::vec3 L = rayOrigin - sphereCenter;  // Vector pointing from sphere center to where ray starts
```

### Step 4: Expand the Distance Formula
Remember: `|vector|² = vector · vector` (dot product with itself)

So `|L + tD|²` becomes `(L + tD) · (L + tD)`:
```
(L + tD) · (L + tD) = r²
```

**Expanding the dot product using the rule: (A + B) · (A + B) = A·A + 2A·B + B·B**
```
L·L + 2t(L·D) + t²(D·D) = r²
```

**In C++ terms:**
```cpp
// L·L = how far ray origin is from sphere center (squared)
float L_dot_L = glm::dot(L, L);

// L·D = how much ray points toward/away from sphere center  
float L_dot_D = glm::dot(L, D);

// D·D = length of ray direction (should be 1 if normalized)
float D_dot_D = glm::dot(D, D);

// The equation becomes:
// L_dot_L + 2*t*L_dot_D + t*t*D_dot_D = r*r
```

### Step 5: Normalize Ray Direction
Since `D` is normalized, `D·D = 1`:
```
L·L + 2t(L·D) + t² = r²
```

Rearranging to standard form:
```
t² + 2t(L·D) + (L·L - r²) = 0
```

**This is now a quadratic equation!** Just like `ax² + bx + c = 0`, but with `t` instead of `x`.

## Quadratic Form

The intersection equation becomes a standard quadratic:
```
at² + bt + c = 0
```

Where:
- `a = 1` (since D is normalized)
- `b = 2(L·D)`
- `c = L·L - r²`

## Solution Using Quadratic Formula

```
t = (-b ± √(b² - 4ac)) / 2a
```

Since `a = 1`, this simplifies to:
```
t = -b/2 ± √(discriminant)/2
```

Where `discriminant = b² - 4c`

## Intersection Cases

The discriminant determines the intersection result:

| Discriminant | Result | Description |
|--------------|--------|-------------|
| `Δ < 0` | No intersection | Ray misses the sphere |
| `Δ = 0` | One intersection | Ray is tangent to sphere |
| `Δ > 0` | Two intersections | Ray enters and exits sphere |

## Implementation Considerations

### Valid Intersections
Only positive `t` values represent valid intersections (forward along the ray).

### Closest Intersection
When two intersections exist, the smaller positive `t` value gives the closest intersection point.

### Computing Intersection Point
Given a valid `t` value:
```cpp
glm::vec3 P = rayOrigin + t * rayDir;
```

### Surface Normal
At intersection point `P`, the surface normal is:
```cpp
glm::vec3 N = glm::normalize(P - sphereCenter);
```

## Optimization Notes

1. **Early Exit**: Check discriminant before computing square root
2. **Avoid Division**: Use `discriminant/4` instead of full quadratic formula when possible
3. **Ray Bounds**: Check `t` values against ray's minimum/maximum bounds
4. **Precision**: Use appropriate floating-point precision for your use case

## Example Pseudocode

```pseudocode
function raySpherIntersect(rayOrigin, rayDir, sphereCenter, sphereRadius):
    L = rayOrigin - sphereCenter
    a = dot(rayDir, rayDir)  // Should be 1 if normalized
    b = 2 * dot(L, rayDir)
    c = dot(L, L) - sphereRadius²
    
    discriminant = b² - 4*a*c
    
    if discriminant < 0:
        return NO_INTERSECTION
    
    sqrtDiscriminant = sqrt(discriminant)
    t1 = (-b - sqrtDiscriminant) / (2*a)
    t2 = (-b + sqrtDiscriminant) / (2*a)
    
    // Return closest positive intersection
    if t1 > 0:
        return t1
    elif t2 > 0:
        return t2
    else:
        return NO_INTERSECTION
```

## Applications

- **Ray Tracing**: Primary ray intersection testing
- **Shadow Rays**: Occlusion testing
- **Collision Detection**: 3D physics simulations
- **Computer Graphics**: Rendering spherical objects
- **Scientific Visualization**: Particle systems and molecular modeling

## Further Reading

- [Real-Time Rendering](http://www.realtimerendering.com/) by Möller, Haines & Hoffman
- [Ray Tracing in One Weekend](https://raytracing.github.io/) by Peter Shirley
- [Physically Based Rendering](https://pbr-book.org/) by Pharr, Jakob & Humphreys

---

*This mathematical foundation forms the basis for efficient sphere intersection testing in modern ray tracing applications.*