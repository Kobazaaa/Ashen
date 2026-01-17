# 🔥 Ashen

Ashen is a lightweight, purpose-built Vulkan renderer created using [VkBootstrap](https://github.com/charles-lunarg/vk-bootstrap).
It was developed specifically for my Graduation Work at Howest University of Applied Sciences - Digital Arts & Entertainment.

The focus of this project is the research and implementation of single atmospheric scattering, extended with ozone absorption. To support this research, I needed a custom Vulkan renderer that offered full control over the rendering pipeline.

Although I initially planned to use my other Vulkan renderer, [Pompeii](https://github.com/Kobazaaa/Pompeii), I ultimately decided against it. Since this work involves performance analysis, I wanted to remove any potential overhead. Pompeii is also intentionally more general and feature-rich than required for this research.

# Controls (main branch)

These controls are only valid on the main branch, as other branches have some controls removed.

| Input | Action |
|----------|----------|
| + / -  | Increase / Decrease sample count   |
| 3 / L-Shift + 3 | Increase / Decrease g parameter for phase functions   |
| 4 / L-Shift + 4 | Increase / Decrease sun intensity   |
| 8 / L-Shift + 8 | Increase / Decrease exposure   |
| 9 / L-Shift + 9 | Switch light preset   |
| R-Shift | Speed up change in value for above inputs  |
| O | Toggle Ozone   |
| F | Toggle Phase Function   |
| Tab | Toggle HDR   |
| WASD | Move Camera Forward / Left / Backward / Right  |
| QE | Move Camera Down / Up  |
| LMB + Drag | Rotate Camera  |
