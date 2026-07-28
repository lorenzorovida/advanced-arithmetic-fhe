import os

# Instead of just 'frames', use a writable absolute path
frames_dir = '/tmp/frames'  # or '/home/youruser/frames'
os.makedirs(frames_dir, exist_ok=True)

import gifos

t = gifos.Terminal(width=600, height=300, xpad=8, ypad=8)


t.gen_prompt(1, 1, 12);

t.gen_typing_text(text="./FlexibleIntsCKKS --ring 16 --bits 128", row_num=1,contin=True,speed=1)

t.gen_text(text="log(QP): 1602, log(N): 14, circuit depth: 30", row_num=2)
t.gen_text(text="\x1b[1m\x1b[33mRunning batched 128-bits operations experiment!\x1b[0m", row_num=4);
t.gen_text(text="a: 333997124454568336686061359039402126756", row_num=5)
t.gen_text(text="b: 90509309311340972773888919803956905857", row_num=6)

t.gen_text(text="\x1b[1;36mAddition (a + b)\x1b[0m", row_num=8)
t.gen_text(text="Expected: 84224066844970845996575671411590821157", row_num=9)
t.gen_text(text="Obtained: 84224066844970845996575671411590821157", row_num=10)

t.gen_text(text="Addition took: 2:562 sec", row_num=11)

t.gen_text(text="\x1b[1;36mComparison (a <= b)\x1b[0m", row_num=13)
t.gen_text(text="Expected: 0", row_num=14)
t.gen_text(text="Obtained: 4.14768e-08", row_num=15)

t.gen_text(text="Comparison took: 2.610 sec", row_num=16)
t.gen_text(text=" ", row_num=17)
t.gen_text(text="\n\x1b[1;36mMultiplication (a * b) % 2^n\x1b[0m", row_num=19)
t.gen_text(text="Expected: 183659073964523510860344451854885583780", row_num=20)
t.gen_text(text="Obtained: 183659073964523510860344451854885583780", row_num=21)

t.gen_text(text="Multiplication took: 19.292 sec", row_num=22)
t.gen_text(text=" ", row_num=23)
t.gen_text(text="\n\x1b[1;36mMultiplication with overflow (a * b)\x1b[0m", row_num=24)
t.gen_text(text="Expected: 30229849046356971678101713184292442072018592292442948506867908337939072809892", row_num=25)
t.gen_text(text="Obtained: 30229849046356971678101713184292442072018592292442948506867908337939072809892", row_num=26)

t.gen_text(text="Multiplication took: 19:542 sec", row_num=27)
t.gen_text(text=" ", row_num=28)
t.gen_text(text="\n\x1b[1;36mLogical shift of mult (a * b << 2)\x1b[0m", row_num=29)
t.gen_text(text="Expected: 120919396185427886712406852737169768288074369169771794027471633351756291239568", row_num=30)
t.gen_text(text="Obtained: 120919396185427886712406852737169768288074369169771794027471633351756291239568", row_num=31)

t.gen_text(text="Logical shift took: 0.322 sec", row_num=32)
t.gen_text(text=" ", row_num=33)

t.gen_prompt(row_num=30, col_num=1, count=12)

t.gen_gif()