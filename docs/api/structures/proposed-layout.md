# ProposedLayout Object

Describes the proposed layout of a `View` and any child Views.

- `size` Size - The size of the View.
- `layouts` Object[] - A list mapping child views to their respective bounds
  - `view` View - The child view whose layout is being proposed.
  - `bounds` Rectangle - The bounds of this child view, within the parent view.
  - `visible` boolean - Whether this child view should be visible.
