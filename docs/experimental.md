# Experimental APIs

Some of Electrons APIs are tagged with `_Experimental_` in the documentation.
This tag indicates that the API may not be considered stable and the API may
be removed or modified more frequently than other APIs with less warning.

## Conditions for an API to be tagged as Experimental

Anyone can request an API be tagged as experimental in a feature PR, disagreements
on the experimental nature of a feature can be discussed in the API WG if they
can't be resolved in the PR.

## Process for removing the Experimental tag

Once an API has been stable and in at least two major stable release lines it
can be nominated to have its experimental tag removed.  This discussion should
happen at an API WG meeting.  Things to consider when discussing / nominating:

* The above "two major stables release lines" condition must have been met
* During that time no major bugs / issues should have been caused by the adoption of this feature
* The API is stable enough and hasn't been heavily impacted by Chromium upgrades
* Is anyone using the API?
* Is the API fulfilling the original proposed usecases, does it have any gaps?
