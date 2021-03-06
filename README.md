# Dorm-Executor

> 基于ESP32单片机，通过WIFI、BLE控制空调、灯光、房门的执行器。
>
> 基于我个人宿舍需求，修改后可支持更多的设备、场景。
>
> 2022\4\6:
>
> 格力空调的BLE小程序控制、QQ机器人控制。

更多功能在不断探索中。。。目前非常简陋，希望各位大佬多多指教。

## 目录

[TOC]

---

## 功能概览

寝室只有一个空调遥控器，用的时候不好找不说，并且往往放在下面的桌子上。谁又愿意下床去拿遥控器呢？这个问题的答案就和谁去关灯、开门一样，都不愿意去。

在上述需求之下，又结合校内定时断电断网的特点，WIFI+BLE方案的优势尽显。在有网有电时，ESP32主动连接寝室内路由器，实现远程遥控，顺便解决忘关空调的环保问题。深夜断电、断网的情况下，使用BLE近场通信，覆盖全屋是没有问题的，一般这时都会在寝室内，也不耽误使用。

重点是两者需要无缝衔接，在恢复WIFI后尽快从BLE模式切换，总是保持较高的可用性。

在使用方面，并不希望引入额外的应用和操作逻辑，这样同学的上手成本会变高。于是BLE模式下选择了微信小程序直接发送进行控制。而WIFI模式下，为了能进行远程操作，搭建了MQTT服务端，又结合之前搭建的QQ机器人，实现在宿舍群内消息控制空调的功能。

具体控制方案请阅读[后文](#user-interface)。

## 工程说明

仓库中目前有两个ESP-IDF 4.4工程：

`ESP32-IRremote-Gree`是仅有蓝牙BLE控制的版本，通过微信小程序即可控制空调。

`ESP32-IRremote-WIFI-BLE`是结合WIFI与BLE的版本，在有可用WIFI时将会**优先**使用WiFi+MQTT+QQ控制，在遇到WIFI、MQTT服务器问题并重试无果后将会切换BLE，并定时重新检查WIFI是否可用。

关于WiFi与BLE模式切换的流程将会在[后文](#comm_schedule)讨论，后期预计会加入模式动态选择功能。

## 红外遥控

### 编码

网络上各种空调编码的教程、项目分享的非常多，但是我试了很久都没成功，所以。。。我拆了一个遥控器并用逻辑分析仪把格力空调的编码抓了出来。和网上的教程大致相同，但**正是细微的不同导致遥控失败**，所以在使用时最好结合实际情况和机型。

由于我寝室使用的是格力空调，目前发送编码仅有这一品牌，同时生成编码的函数功能我只保留了我认为常用的一些功能，有需要的朋友可以自行拓展，后续有需求也可以继续完善。

**关于编码格式的详细内容以及逻辑分析仪波形图片都会在文章里详细讲述。**

### 程序实现

各个工程使用的都是相同的源文件`gree.c`与头文件`gree.h`。

他们提供了层层包装的函数来完成不同级别的调用，其他应用程序可以直接调用最抽象的发送函数并输入需求字符串，即可完成红外遥控，也可以按需调用二进制码生成、红外码生成、红外RMT发送等分步函数。

由以下函数组成：

- `generate_gree_cmd`按照温度、模式等参数生成由01组成的序列
- `generate_gree_item`将上一个函数生成的二进制码转换为RMT所需要的items序列
- `level_implement`被上一个函数所调用的内联函数，用于填充items
- `send_gree`包装了前三个函数并发送红外码，在内部初始化RMT、发送item序列、卸载RMT
- `remote_control`接收字符串并解析调用`send_gree`

**起始码、连接码、各类电平的持续时间、红外LED所在IO口等等遥控相关参数都可以在`gree.h`的宏定义修改。**

## 连接模式

`ESP32-IRremote-Gree`是仅有蓝牙BLE控制的版本，通过微信小程序即可控制。

`ESP32-IRremote-WIFI-BLE`是结合WIFI与BLE的版本，WIFI模式下通过MQTT与QQ机器人实现控制，不需要额外的手机应用。

### 蓝牙BLE

正如之前[《ESP32:蓝牙BLE控制M3508电机》](https://www.cnblogs.com/huxiaoan/p/15861624.html)中所使用的BLE GATT SERVER，这里拿过来用于触发控制并且传输控制值。

在收到控制指令后，在蓝牙事务处理回调的接收事件部分将会发送控制字符串到红外遥控队列，一直被此队列堵塞的红外控制任务接收到控制信息后进行解析、生成、发送。再进入下一次阻塞。

蓝牙相关函数都放在`ble.c`与`ble.h`这两个文件中，具体发送的指令将会在[指令格式](#instruction_format)一节讨论。

#### 蓝牙相关函数

- `ble_init`BLE初始化函数
- `ble_deinit`BLE关闭函数
- `gatts_profile_a_event_handler`BLE事务处理回调

### WIFI

WiFi相关函数都实现在了`main.c`，并没有单独出来，头文件`comm_schedule.h`包含了WIFI配置的相关宏定义：

``` c
#define IRremote_WIFI_SSID "the_ssid"	//要连接到的WIFI名称
#define IRremote_WIFI_PASS "the_pwd"	//对应的密码
#define WIFI_HOSTNAME "ESP32-IRremote"	//在局域网中的主机名，可以在路由器后台看到
#define WIFI_MAX_RETRY 3    			//意外断开、连接失败时连接重试的上限次数
```

在上电时，主函数将会自动尝试连接WIFI，将WiFi作为首选的连接模式，之后的连接将会被调度任务接管，具体策略将会在[切换策略](#comm_schedule)一节中讨论。

### MQTT与QQ控制

在WiFi连接下，直接使用TCP协议进行消息传输并不方便，尤其是对手机控制不友好。我希望能在不引入更多的APP的情况下进行控制，并且要足够方便。

于是我想到了之前搭建的QQ机器人，利用QQ结合MQTT服务器完成控制信息的传达。同时方便室友一起使用，使用成本极低。我个人使用的QQ机器人与MQTT Broker分别运行于两台云服务器上，QQ机器人使用的是开源的[OPQBOT](https://github.com/opq-osc)，其插件开发极其简单，非常适合本应用，以及整点花活。

如果不需要QQ控制，可以仅使用MQTT服务并配合手机端MQTT应用即可。

MQTT相关函数都实现在了`main.c`，并没有单独出来，头文件`comm_schedule.h`包含了MQTT配置的相关宏定义：

``` c
#define IRremote_MQTT_URI "URI"             //MQTT服务器地址，示例：mqtt://xxx.xxx.xxx.xxx
#define IRremote_MQTT_USR "the username"    //MQTT 用户名
#define IRremote_MQTT_PWD "the password"    //对应密码
#define MQTT_MAX_RETRY 3                    //意外断开、连接失败时连接重试的上限次数
```

由于WIFI与MQTT的启动和关闭是紧密相关的，两者的启动逻辑会在[切换策略](#comm_schedule)一节中讨论。

### <a name="comm_schedule">切换策略</a>

切换策略指的是通信方式的调度策略，包括WIFI\MQTT的启动、在WIFI或者MQTT失效时重试并切换蓝牙，在蓝牙模式下检查WIFI是否可用等一系列调度，目的在于尽可能的保证设备可用。因为学校的环境会频繁的断电、断网导致WIFI模式并不总是可行。

上述的行为主要由任务函数`mode_schedule_task`配合一个软件定时器、一个消息队列以及WIFI\MQTT\BLE事件处理回调的部分逻辑组成。

![Dorm-Executor-schedule](pic/Dorm-Executor-schedule-16492517522712.png)

大致流程图如上。

有一处没有在图中说明的是，因为在发生意外时，WIFI、MQTT事件处理回调都可能在运行，类似WiFi断开的事件同样会引起MQTT的断开，如果两者在尝试重连之后都向调度任务发送切换到BLE模式的队列消息，会引发错误，实际程序使用标志保证两者不会重复发送。

## <a name="user-interface">用户控制方案</a>

无论是BLE还是WIFI，都采用传输字符串的形式进行控制。每一层通信可以加入自己需要的前缀用于识别，并在传递给下一层时将其去除。

### BLE

使用任一微信BLE调试小程序即可进行通信，发送字符串形式的消息。也可以使用Nordic Connect调试助手，但是貌似麻烦一些。

### WIFI\MQTT\QQ

我使用的`OPQBot`的对于插件开发非常友好，特别是其中的Python接口，简单的对话逻辑几分钟就能搞定。同时还能自带功能查询。

QQ机器人依靠检查消息的前缀来判断消息是否是控制指令，如果符合，就会向MQTT服务器的对应Topic发送去除前缀的控制字符串，接下来，联网的单片机就能收到控制信息。

<img src="pic/image-20220408100656340.png" alt="关闭空调" style="zoom:33%;" />

<img src="pic/image-20220408100732828.png" alt="用法查询" style="zoom:33%;" />

### <a name="instruction_format">指令格式</a>

**BLE模式**：onC25表示开机、制冷模式、25度。onH26代表开机、制热、26度。off代表关机。

**MQTT**：向MQTT话题发送的内容格式与上述BLE一样

**QQ**：在前面的基础上要加入`greeac `的前缀，要有空格分开。

**可以去掉开启指令中的C和H，默认为制冷。**

