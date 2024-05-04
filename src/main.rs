#![no_std]
#![no_main]
#![feature(type_alias_impl_trait)]

use core::{fmt::Write, str::FromStr};

use esp_hal::{
    embassy,
    peripherals::Peripherals,
    prelude::*,
    rng::Rng,
    timer::TimerGroup,
    clock::ClockControl,
};

use esp_wifi::{
    initialize,
    wifi::{WifiController, WifiDevice, WifiEvent, WifiStaDevice, WifiState, Configuration, ClientConfiguration},
    EspWifiInitFor,
};

use esp_backtrace as _;
use esp_println::println;

use embassy_executor::Spawner;
use embassy_net::{tcp::TcpSocket, Config, Stack, StackResources};
use embassy_time::{Duration, Timer};

use static_cell::make_static;

use heapless::String;

const SSID: &str = env!("SSID");
const PASSWORD: &str = env!("PASSWORD");

#[main]
async fn main(spawner: Spawner) {
    let mut peripherals = Peripherals::take();
    let system = peripherals.SYSTEM.split();
    let clocks = ClockControl::max(system.clock_control).freeze();
    let timer = esp_hal::systimer::SystemTimer::new(peripherals.SYSTIMER).alarm0;

    let mut rng = Rng::new(&mut peripherals.RNG);
    let seed: u64 = rng.random().into();

    let init = initialize(
        EspWifiInitFor::Wifi,
        timer,
        Rng::new(peripherals.RNG),
        system.radio_clock_control,
        &clocks,
    )
    .unwrap();

    let wifi = peripherals.WIFI;
    let (wifi_interface, controller) =
        esp_wifi::wifi::new_with_mode(&init, wifi, WifiStaDevice).unwrap();

    let timer_group0 = TimerGroup::new_async(peripherals.TIMG0, &clocks);
    embassy::init(&clocks, timer_group0);

    let config = Config::dhcpv4(Default::default());

    // Init network stack
    let stack = &*make_static!(Stack::new(
        wifi_interface,
        config,
        make_static!(StackResources::<3>::new()),
        seed
    ));

    // Execute embassy tasks
    spawner.spawn(connection(controller)).ok();
    spawner.spawn(net_task(stack)).ok();
}

#[embassy_executor::task]
async fn connection(mut controller: WifiController<'static>) {
    loop {
        if esp_wifi::wifi::get_wifi_state() == WifiState::StaConnected {
            // wait until we're no longer connected
            controller.wait_for_event(WifiEvent::StaDisconnected).await;
            Timer::after(Duration::from_millis(5000)).await
        }

        if !matches!(controller.is_started(), Ok(true)) {
            println!("Attempting to connect to {}...", SSID);
            let client_config = Configuration::Client(ClientConfiguration {
                ssid: String::from_str(SSID).unwrap(),
                password: String::from_str(PASSWORD).unwrap(),
                ..Default::default()
            });
            controller.set_configuration(&client_config).unwrap();
            controller.start().await.unwrap();
        }

        match controller.connect().await {
            Ok(_) => println!("Connected.\n"),
            Err(e) => {
                println!("Failed to connect to wifi: {e:?}");
                Timer::after(Duration::from_millis(5000)).await
            }
        }
    }
}

#[embassy_executor::task]
async fn net_task(stack: &'static Stack<WifiDevice<'static, WifiStaDevice>>) {
    stack.run().await
}
