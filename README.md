# natechoe.dev generator

A very simple tool to generate natechoe.dev pages. It's effectively just a
generic file preprocessor with HTML some auto escaping.

Please not that ncdg minifies HTML so these examples aren't really correct.

## Usage

There are only 3 features in ncdg:

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

### Automatic escaping

```
<pre><code>%\
#include <stdio.h>
%</code></pre>
```

Result:

```
<pre><code>
#include &lt;stdio.h%gt;
</code></pre>

Note that text inside of escaped sections are not minified.
