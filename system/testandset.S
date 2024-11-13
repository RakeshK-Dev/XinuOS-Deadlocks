/* testandset.S - test and set the value */

		.text
		.globl	test_and_set

 test_and_set:

            pushl %ebp              /* Push ebp onto stack		*/
            movl %esp, %ebp         /* Record current SP in ebp	*/

            pushl %edx              /* Push edx onto stack		*/
            movl 8(%ebp), %edx      /* copy *ptr into edx */
            movl 12(%ebp), %eax     /* copy new_value into eax */
            xchg (%edx), %eax       /* atomic swap operation of *ptr and new_value */

            popl %edx               /* restore value of edx register  */
            popl %ebp               /* restore ebp to previous stack frame  */

            ret                     /* return to caller stack frame  */