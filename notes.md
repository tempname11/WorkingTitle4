### larger things to do
[ ] G-Pass for real
[ ] L-Pass for real (sun)
[ ] remove "example" from naming
[ ] fix the intermittent bug

### architectural problems
* "lots of text": hard to navigate and understand flow of code
* struct member init in-place (has this member been already initialized or not?)
* scheduling is dynamic and costly, but it shouldn't really be
* barrier code is brittle with regard to new/changed passes