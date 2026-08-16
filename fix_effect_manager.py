# -*- coding: utf-8 -*-
"""修复效应管理器.cpp：1) event_send 语法错误 2) use-after-erase 隐患"""
import io

path = r'D:\代码存储\代码仓库\游戏引擎\游戏引擎\src\core\effect\Effect_Manager\core\效应管理器.cpp'

with io.open(path, encoding='utf-8', newline='') as f:
    content = f.read()

uses_crlf = '\r\n' in content
print('CRLF' if uses_crlf else 'LF', '文件行尾')

def replace_once(old, new, tag):
    global content
    variants = [old]
    if uses_crlf:
        variants.append(old.replace('\n', '\r\n'))
    for v in variants:
        if v in content:
            content = content.replace(v, new if not uses_crlf else new.replace('\n', '\r\n'), 1)
            print('[OK]', tag)
            return True
    print('[FAIL]', tag)
    # 调试：打印附近内容
    idx = content.find('event_send')
    if idx >= 0:
        print('  上下文:', repr(content[max(0,idx-80):idx+80]))
    return False

# 1) event_send(events,) 语法错误 → 补 acl_key
ok1 = replace_once(
    '\t\tevent_terminal.event_send(events,);',
    '\t\tevent_terminal.event_send(events, acl_key);',
    'event_send 补 acl_key')

# 2) remove_effect 中 erase 后访问 effect_set[index] 的 use-after-erase
old2 = ('\t\t\teffect_set.erase(effect_set.begin() + index);\n'
        '\t\t\t//匹配效应分组\n'
        '\t\t\tfor (int group_time = 0; group_time < group_effect_set.size(); group_time++)\n'
        '\t\t\t{\n'
        '\t\t\t\t//简化表示路径\n'
        '\t\t\t\tauto& group = group_effect_set[group_time];\n'
        '\t\t\t\t//若成功匹配效应分组\n'
        '\t\t\t\tif (group.inclusion == effect_set[index].inclusion)')
new2 = ('\t\t\t//擦除前先保存所属分组标记（erase 后索引失效，不能再访问 effect_set[index]）\n'
        '\t\t\tauto erased_inclusion = effect_set[index].inclusion;\n'
        '\t\t\teffect_set.erase(effect_set.begin() + index);\n'
        '\t\t\t//匹配效应分组\n'
        '\t\t\tfor (int group_time = 0; group_time < group_effect_set.size(); group_time++)\n'
        '\t\t\t{\n'
        '\t\t\t\t//简化表示路径\n'
        '\t\t\t\tauto& group = group_effect_set[group_time];\n'
        '\t\t\t\t//若成功匹配效应分组\n'
        '\t\t\t\tif (group.inclusion == erased_inclusion)')
ok2 = replace_once(old2, new2, 'remove_effect use-after-erase 修复')

with io.open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(content)
print('=== 完成：', ok1, ok2, '===')
