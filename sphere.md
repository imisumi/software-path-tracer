# Ray-Sphere Intersection Mathematical Derivation

## Overview
This document provides a complete mathematical derivation for finding the intersection points between a ray and a sphere in 2D/3D space. This is a fundamental operation in computer graphics, ray tracing, and collision detection.

## Ray Definition
A ray is defined by an origin point and a direction vector:

- **O** = origin point
- **D** = direction vector (usually normalized)
- **t** = distance parameter along the ray (scalar, t ≥ 0)
- **P** = any point on the ray

The parametric equation of a ray is:
```
P = O + tD
```

In component form:
```
Px = Ox + tDx
Py = Oy + tDy
Pz = Oz + tDz  (for 3D)
```

## Sphere Definition
A sphere is defined by its center and radius:

- **C** = center of the sphere
- **r** = radius of the sphere
- **P** = any point on the sphere surface

The implicit equation of a sphere is:
```
|P - C|² - r² = 0
```

Where `|P - C|` means the **magnitude** (length) of the vector from C to P. This is equivalent to the distance from point P to the center C.

## Understanding Vector Magnitude Notation | |

The `| |` symbols around a vector represent its **magnitude** (also called **length** or **norm**). This is a fundamental concept that deserves clear explanation:

### What Does |v| Mean?
- **Verbally**: "The magnitude of vector v" or "the length of vector v"
- **Conceptually**: How long the vector arrow is
- **Mathematically**: The distance from the vector's tail to its head

### Mathematical Definition
For a vector **v = (vx, vy)** in 2D:
```
|v| = √(vx² + vy²)
```

For a vector **v = (vx, vy, vz)** in 3D:
```
|v| = √(vx² + vy² + vz²)
```

### Concrete Example
Given vector **D = (3, 4)**:
- **|D|** = √(3² + 4²) = √(9 + 16) = √25 = 5
- **|D|²** = 5² = 25 (or directly: 3² + 4² = 9 + 16 = 25)

### In Programming (GLM)
```cpp
glm::vec3 D = glm::vec3(3.0f, 4.0f, 0.0f);
float magnitude = glm::length(D);        // |D| = 5.0
float magnitudeSquared = glm::dot(D, D); // |D|² = 25.0 (more efficient!)
```

### Why We Need This Notation
Without `| |`, writing D² would be ambiguous - it could mean:
- Square each component: (Dx², Dy², Dz²) - gives a vector
- Something else entirely

With `|D|²`, it's crystal clear we want the **scalar** value Dx² + Dy² + Dz².

In component form (2D circle shown, extends naturally to 3D):
```
(Px - Cx)² + (Py - Cy)² - r² = 0
```

## Intersection Derivation

### Step 1: Define Vector L for Cleaner Notation
Let's introduce **L = O - C** (vector from sphere center to ray origin) to simplify our algebra:
```
L = O - C
Lx = Ox - Cx
Ly = Oy - Cy
```

### Step 2: Substitute Ray Equation into Sphere Equation
Replace P in the sphere equation with the ray equation P = O + tD:
```
(Ox + tDx - Cx)² + (Oy + tDy - Cy)² - r² = 0
```

Since L = O - C, we have Lx = Ox - Cx and Ly = Oy - Cy, so:
```
(Lx + tDx)² + (Ly + tDy)² - r² = 0
```

### Step 3: Expand the Squared Terms
For the x-component:
```
(Lx + tDx)² = Lx² + 2tLxDx + t²Dx²
```

For the y-component:
```
(Ly + tDy)² = Ly² + 2tLyDy + t²Dy²
```

### Step 4: Substitute Back and Collect Terms
```
Lx² + 2tLxDx + t²Dx² + Ly² + 2tLyDy + t²Dy² - r² = 0
```

Rearranging by powers of t:
```
t²(Dx² + Dy²) + 2t(LxDx + LyDy) + (Lx² + Ly² - r²) = 0
```

### Step 5: Standard Quadratic Form
This gives us a quadratic equation in the form **at² + bt + c = 0**, where:

```
a = Dx² + Dy² = |D|²  (magnitude squared of direction vector)
b = 2(LxDx + LyDy) = 2L · D  (2 times dot product of L and D)
c = Lx² + Ly² - r² = |L|² - r²  (magnitude squared of L vector minus radius squared)
```

### Step 6: Solve Using Quadratic Formula
```
t = (-b ± √(b² - 4ac)) / (2a)
```

The discriminant **Δ = b² - 4ac** determines the nature of intersections:
- **Δ > 0**: Two intersection points (ray enters and exits sphere)
- **Δ = 0**: One intersection point (ray is tangent to sphere)
- **Δ < 0**: No intersection (ray misses sphere)

## Vector Form (Generalized)

For any dimension, the coefficients become:

```
L = O - C  (vector from sphere center to ray origin)
a = D · D = |D|²
b = 2L · D
c = |L|² - r²
```

Where · represents the dot product.

## Implementation Notes

### Optimizations
1. If the direction vector D is normalized (|D| = 1), then a = 1
2. Pre-compute L = O - C to avoid repeated subtraction
3. Check discriminant before computing square root

### Edge Cases
- Ray origin inside sphere: One or two positive t values
- Ray origin on sphere surface: One t = 0, one positive t
- Direction vector with zero magnitude: Degenerate case

### Example Pseudocode
```pseudocode
function rayIntersectSphere(origin, direction, center, radius):
    L = origin - center  // Vector from sphere center TO ray origin
    a = dot(direction, direction)
    b = 2.0 * dot(L, direction)
    c = dot(L, L) - radius * radius
    
    discriminant = b * b - 4 * a * c
    
    if discriminant < 0:
        return NO_INTERSECTION
    
    sqrt_discriminant = sqrt(discriminant)
    t1 = (-b - sqrt_discriminant) / (2 * a)
    t2 = (-b + sqrt_discriminant) / (2 * a)
    
    return [t1, t2]  // Filter for t >= 0 as needed
```