import asyncio

import websockets, pygame, time

pygame.init()

global useJoysticks
useJoysticks = False

async def main():
    if pygame.joystick.get_count() > 0:
        global useJoysticks
        useJoysticks = True

        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        num_axes = joystick.get_numaxes()

        print(f"Joystick detected: {joystick.get_name()}")
        print(f"Number of axes: {num_axes}")
    else:
        print("Using keyboard controls (WASD or Arrow Keys)")
        background_colour = (234, 212, 252)

        screen = pygame.display.set_mode((300, 300))

        pygame.display.set_caption('Keyboard Control')

        screen.fill(background_colour)

        pygame.display.flip()
        
    print("Enter XRP WSS URL\nws://", end="")
    ip = input()

    try:
        wss = await asyncio.wait_for(websockets.connect("ws://" + ip), timeout=10)
    except asyncio.TimeoutError:
        print("Connection timed out.")
        return
    
    while True:
        pygame.event.pump()
        
        if useJoysticks:
            power = -joystick.get_axis(1)
            turn = joystick.get_axis(2)
            
            left_shoulder = joystick.get_button(4)
            right_shoulder = joystick.get_button(5)
        
        
            if right_shoulder:
                servo = 255
            elif left_shoulder:
                servo = 127
            else:
                servo = 0
        else:
            keys=pygame.key.get_pressed()
            
            power = 1 if keys[pygame.K_UP] else -1 if keys[pygame.K_DOWN] else 0
            turn = 1 if keys[pygame.K_RIGHT] else -1 if keys[pygame.K_LEFT] else 0
            servo = 127 if keys[pygame.KMOD_SHIFT] else 0

        data = {
            "power": power,
            "turn": turn,
            "servo": servo
        }

        await asyncio.wait_for(wss.send(str(data)), timeout=1)
        
        if (abs(power) > 0.1 or abs(turn) > 0.1):
            await asyncio.sleep(0.05)
        else:
            await asyncio.sleep(0.2)


if __name__ == "__main__":
    asyncio.run(main())