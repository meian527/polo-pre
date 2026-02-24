# polo lang 基本语法

### 基本函数定义，控制流
```polo
import std;
import std::io; 
// 查找 ${PACKAGE_PATH}/std/io.polo
// or ${PACKAGE_PATH}/std/io/package.polo
// 其中PACKAGE_PATH主要包括
// 1. 项目配置文件Polo.toml所在位置/src
// 2. 工具链安装路径/packages
// 3. ...


fn main() -> i32 {
    const pi: f64 = 3.14159;
    let mutable: i32 = 4;
    if mutable > 3 {
        io::println("mutable > 3");
    } else if mutable == 3 {
        io::println("mutable = 3");
    } else {
        io::println("mutable < 3");
    }
    
    /*
     * smart_cast!<T> 用于聪明基本类型的转换
     * 因为一般转换 (T)v 会直接从v的地址位置截取新的T大小的值返回，这一个函数会加以处理
     * 
     */
     
    // while式循环
    for std::smart_cast<bool>(mutable) {
        std::libc::printf("%d\n", mutable);
        mutable -= 1;
    }
    
    // 范围for循环
    for i <: [1,4,7,8,9]: i32[] {
        std::libc::printf("%d\n", i);
    }
    
    // loop式循环
    for {
        if true {break;}
    }
    
    // 多分支语句
    let value: i32 = 4;
    let result: i32 = must_inline!(std::None<i32>()); 
    match value {
        1 => {}
        2 => {}
        3 => {}
        4 => {}
        _ => {}
    }
    
    return 0;
}
```

### 数据类型与全局变量
```polo
// 整形有i8~i64 暂无unsigned整形， 浮点型只有f64
/* 
    基本字符串类型是str，占用16字节（pointer + length)
    变量 and 常量的类型修饰用 name: type 
    函数用 fn name() -> type;
    
*/
static let GLOBAL_VAR: f64 = 0.5;
static const GLOBAL_CONST: i32 = 3;
static const GLOBAL_STR: str   = "GLOBAL";
```

### 枚举，结构体，接口，残疾的反射
```polo
import std;

// pub关键字修饰后只要import此package的文件都可以使用
// 否则只有当前文件可用
pub trait ToString {
    fn to_string(self: &Self) -> std::String;
}
#!(ToString)    // 编译器自动实现此类内置trait
enum Color {
    Red, Green, Blue, Yellow,
} impl { // 自动隐式赋名与上enum同名,也可以手动赋名

}
static let red_color: Color = Color::Red;

#!(ToString)
#!(Packed)  //
struct Struct {
    color: Color;
} impl {    // 自动隐式赋名与上struct同名,也可以手动赋名
    constructor(self: *Self, color: Color) {
        self.color = color;
    }
    fn print(self: *Self) -> void {
        std::io::println(self.to_string());
    }
}

```
### 宏函数
```polo
// 警告： 宏函数等同于c define(), 是简单文本替换

// mc_xxx!是标准库提供的一系列编译期宏
macro print(...) {
    $s: StrLiteral;
    (...) => {
        mc_for!($v <: ... , { // mc_for可以遍历不定参列表
            mc_if!(mc_type_is!(i32), {
                $s = $s + "%d";
                mc_continue!();
            });
            mc_if!(mc_type_is!(i64), {
                $s = $s + "%lld";
                mc_continue!();
            });
            mc_if!(mc_type_is(StrLiteral), {
                $s = $s + $v;
            });
            
            // 省略
            
            $s = $s + " ";
        })
    }
    mc_if!(mc_not_imported!(std), { import std; }); // 局部import
    
    #!(discard) std::libc::printf($s, ...);
}
macro println(...) {
    print!(..., "\n");
}
```
### 泛型
```polo
struct Encap<T> {
    v: T;
} impl {}

struct Encap2<T <: #!(ToString)> {    // 要求泛型实现ToString trait
    v: T;
} impl {
    pub fn print(self: *Self) {
        println!(self.v.to_string());
    }
}

struct Encap3<T = i32> {    // 默认类型
    v: T;
}


enum Option<T> {    // polo不提倡使用这种判空方式
    Some(T),
    None,
}
```
