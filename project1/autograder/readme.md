# Usage
Place your `bit-shifting.c` file in the same directory as `autograder.c`, `makefile`, and `rungrader.sh`. Then run `rungrader.sh`. If the code compiled correctly and the program didn't crash, a report file called `autograder_out.txt` is generated.

# Other Info

The autograder will check correctness of `set_bit_at_index()`, `get_bit_at_index()`, and `get_top_bits()`. As defined in `makefile`, the grader has 2 test targets. One will use the default bitmap size and `myaddress` value. The other will use a larger bitmap size and different `myaddress` value.

# Tests
The test for `get_top_bits()` consists of returning the corresponding value for 1-9 of the top bits of `myaddress`. The test for the bitmap functions consist of setting and getting all even bits in the bitmap.

# Scores
Each test case is worth an equal portion of the total score for that test. For example, if the test for `set_bit_at_index()` is 
worth 4 points in total, and the bitmap has 32 bits, then 32 test cases exist (16 even indices to check for 1, 16 to check for 0).
Thus, each test case is worth 0.125 points.

Total Points:
- `set_bit_at_index`: 4.0
- `get_bit_at_index`: 4.0
- `get_top_bits`: 2.0

The 3 tests above are run twice - for the different bitmap size and `myaddress` value.
The maximum score is 20.0.