# CalculatedLayoutConfig Object

* `calculateProposedLayout` Function\<ProposedLayout\> - A function that will be invoked whenever a new calculated view layout is needed.
  * `maxSize` Size - The size within which the view and its children (if any) should be laid out

When `View.setLayout` is passed a `calculateProposedLayout` callback, that callback is invoked any time the view manager needs to update the view's layout.
The callback should synchronously return a `ProposedLayout` object with the size of the current view, and a list of entries for each child view, specifying that child view's proposed size and position.
