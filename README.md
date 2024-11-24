# PLCLToWeb

PLCLToWeb is a Papyrus(Like)ConfigurationLanguage to HTML & CSS transpiler.

Why does this exist? No one stopped me.

## TODO

- [ ] Generate templates for validation
- [ ] Probably write better documentation
- [ ] Make the code more readable
- [x] Allow for non-minified output

## Special Elements & Attributes

### HTML

#### `_Text` Element

Only takes the `Content` attribute, outputs text as is.

Example:
```plcl
ConfigElement p
    ConfigList Elements
        ConfigListElement 0
            ConfigElement _Text
                Content = "meow"
            endConfigElement
        endConfigListElement
    endConfigList
endConfigElement
```
results in
```html
<p>meow</p>
```

#### `Doctype` Element

Only takes the `Content` attribute, outputs a doctype.

Example:
```plcl
ConfigElement Doctype
    Content = "html"
endConfigElement
```
results in
```html
<!DOCTYPE html>
```

#### `Elements` List

Every standard element takes this.

Contains children elements.

Example: [The _Text Element example](#_text-element)

### CSS

#### `_Type` Attribute

Can be `Tag`, `Class`, or `Id`.

Specifies the type of the selector.

Defaults to `Tag`, although it's recommended to specify it.

#### `_Templates` List

In the top level, contains CSS templates.

Example:
```plcl
ConfigList _Templates
    ConfigListElement 0
        ConfigElement Example
            Padding = "20px"
        endConfigElement
    endConfigListElement
endConfigList
```

Inside a CSS element, puts the template attributes in the element.

Example:
```plcl
ConfigElement h1
    _Type = "Tag"
    Margin = "20px"
    ConfigList _Templates
        ConfigListElement 0
            ConfigElement Template
                Name = "Example"
        endConfigListElement
    endConfigList
endConfigElement
```
results in
```css
h1 {
    padding: 20px;
    margin: 20px;
}
```

#### `_Pseudoclasses` & `_Pseudoelements` Lists

Inside a CSS element, makes a pseudo-class or pseudo-element with the name from the inside element.

Example:
```plcl
ConfigElement a 
    _Type = "Tag"
    Color = "blue"
    ConfigList _Pseudoclasses
        ConfigListElement 0
            ConfigElement Hover
                Color = "red"
            endConfigElement
        endConfigListElement
    endConfigList
endConfigElement
```
results in
```css
a {
    color: blue;
}

a:hover {
    color: red;
}
```

## General Information

- Element names are output as is.
- Attribute names are output lowercase with a `-` between words(capital letters minus the first).
- Special attribute names are case-insensitive
- Template names are case-sensitive

## Building and Installing

### Requirements

- A C++23 compiler 
- CMake 3.10 or higher
- (Optional) Already installed [libPLCL](https://github.com/PapyrusLikeConfigurationLanguage/libPLCL) (if not, it will be acquired automatically with FetchContent)

### Building

```bash
mkdir build 
cd build
cmake .. # -DCMAKE_INSTALL_PREFIX=/path/to/install, defaults to /usr/local
make
```

### Installing

```bash
make install
```

## Examples

Check the [examples](examples) directory for examples.
