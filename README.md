# natechoe.dev generator

A very simple tool to generate natechoe.dev pages. It's effectively just a
generic file preprocessor with HTML some auto escaping.

Please not that ncdg minifies HTML so these examples aren't really correct.

## Usage

There are only 4 features in ncdg:

### Include statements

file1.html

```
<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'>
		<link rel='stylesheet' href='/resources/style.css'>
	</head>
	<body>
		@%file2.html@
	</body>
</html>
```

file2.html

```
<h1>Title!</h1>
<p>Content!</p>
```

Result:

```
<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'>
		<link rel='stylesheet' href='/resources/style.css'>
	</head>
	<body>
<h1>Title!</h1>
<p>Content!</p>
	</body>
</html>
```

### Variables

file1.html

```
<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'>
		<link rel='stylesheet' href='/resources/style.css'>
		<title>%!title%</title>
	</head>
	<body>
		<h1>@!title@</h1>
		@%file2.html@
	</body>
</html>
```

file2.html

```
@=title Title!@
<p>Content!</p>
```

Result:

```
<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'>
		<link rel='stylesheet' href='/resources/style.css'>
		<title>Title!</title>
	</head>
	<body>
		<h1>Title!</h1>
<p>Content!</p>
	</body>
</html>
```

A variable expansion can go through multiple variables until one is found.

file.html

```
@!var1,var2,var3@
@=var2 value@
```

Result:

```
value
```

var1 doesn't exist, so ncdg moves on to var2, which does exist, and skips var3.

### Automatic escaping

```
<pre><code>@\
#include  <stdio.h>
@</code></pre>
```

Result:

```
<pre><code>#include  &lt;stdio.h&gt;
</code></pre>
``

Note that text inside of escaped sections is not minified. Also note that the
first character after the code is swallowed.

### Excluding minification

```
@&
&
this text isn't minified
@
```

Result:

```
&
this text isn't minified
```

Used for legacy web pages on my site that I don't want to update
