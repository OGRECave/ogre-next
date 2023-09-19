# Resolving Merge Conflicts in Ogre-Next 3.0 {#ResolvingMergeConflicts30}

In Ogre-Next 3.0 we performed multiple changes that affected nearly the entire code base in subtle ways:

 - Codebase was C++11-fied (e.g. added `override` and `final` keywords)
 - `SharedPtr` was replaced with `std::shared_ptr` and lots of its code was deprecated
 - Lots of warnings were fixed
 - Clang format 13 was applied to the entire codebase

Users who run a forked/customized version of Ogre-Next may found rebasing to the latest version a near impossible job due to the sheer amount of *minor* merge conflicts.

To perform this task more easily, we recommend the following:

 1. Merge your changes with `master` branch (or 3.0 when it appears)
 2. Resolve all conflicts with `your code`
 3. Apply clang-format-13 again
 4. Inspect the diff again to check for any auto-merge mistakes and fix them
 5. Rebuild code to ensure it still compiles and runs
 6. Commit

Now you have your changes applied on top of Ogre-Next 3.0!

## Notes:

We're using clang-format-13

A common issue is that auto-merging will skip either an opening or closing brace (i.e. missing `{` or `}`)

If you know what to look for, you can spot this quite easily thanks to clang format. Let say the following code:

```cpp
void myFuntion( int a )
{
	for( int i = 0; i < count; ++i )
	{
		if( condition )
		{
			anotherFunction( a );
		}
	}
}

void anotherFunction( int a );
void run();
void moreFunctions()
{
	int abc = 0;
}
```

Got merged with but now a `}` is missing:

```cpp
void myFuntion( int a )
{
	for( int i = 0; i < count; ++i )
	{
		if( condition )
		{
			anotherFunction( a );
		}
}

void anotherFunction( int a );
void run();
void moreFunctions()
{
	int abc = 0;
}
```

After applying Clang format, everything that follows will indent (and likely the code will not compile) which make much easier to spot where the problem went wrong:

```cpp
void myFuntion( int a )
{
	for( int i = 0; i < count; ++i )
	{
		if( condition )
		{
			anotherFunction( a );
		}
	}

	void anotherFunction( int a );
	void run();
	void moreFunctions()
	{
		int abc = 0;
	}
```

If you find after merging and applying clang format that there is massive indentation being added (or removed); start from the top and find the first occurrence of this change and start looking for the missing brace.


# Batch Script

The following bash script will run clang-format on all files with the right extensions in the current folder and recursively in its sub-directories.

```bash
FILES=$(find . -name "*.mm" -o -name "*.h" -o -name "*.cpp")

for file in $FILES
do
  echo "Formatting \"$file\""
  clang-format-13 -i "$file"
done
```

You can also look at our python script in [.github/workflows/run_clang_format.py](https://github.com/OGRECave/ogre-next/blob/879a86fd4d1e7274af67b37628f29d6ad06a7d79/.github/workflows/run_clang_format.py) if you prefer Python instead.