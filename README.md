# Rollback Networking In a UE5 Non-Fighting Game
My Honours project's UE5 project files. I aim to use GGPO, Bullet3 Physics, and Unreal Engine 5 to create an FPS game demo that uses Rollback Netcode.

WARNING: If you intend to clone/use this repo for your own use keep in mind it has a few major bugs/issues that need resolving. As you'd expect with Rollback the issues are related to performance and desyncs.
I haven't had the time to go back and look into these as I've been hired as a game dev since this University project. But my first guess would be to look at the saving/loading of the Bullet3 physics as I had to rush the implementation (Bullet3 python has functions for this, but the C++ lib didn't unexpectedly).
If you want to learn more or find what sources/repos I used to assemble this I'd recommend checking out my dissertation on my portfolio (PDF link can be found on this page: https://mcguinnessdeclan0.wixsite.com/game-dev-portfolio/server-rollback-netcode ).
