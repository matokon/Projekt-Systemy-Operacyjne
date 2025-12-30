# Cable Car Lift – Simulation (IPC / Processes)

A simulation of a summer chairlift system in a mountain resort town. The system models pedestrians and downhill bikers, ticket/pass sales, station gates, platform access control (grouping people onto chairs), emergency stop/resume coordination between employees, and end-of-day usage reporting.

## Problem Summary (from assignment)

- The lift uses **4-seat chairs**, **72 chairs total**.
- At most **36 chairs can be occupied simultaneously**.
- Allowed chair composition:
  - **max 2 bikers**, or
  - **1 biker + 2 pedestrians**, or
  - **4 pedestrians**
- People arrive at random times (not all must use the lift).
- Access requires paying for a **ticket/pass** at the cashier.
- Pass types: **single-use**, **time-based** (Tk1, Tk2, Tk3), or **daily**.
- **25% discount** for:
  - children **under 10**
  - seniors **over 65**
- Children **under 8** are under constant adult supervision.
- Two gate layers:
  - **Lower station entry**: **4 gates in parallel**, pass validation.
  - **Platform entry**: **3 gates in parallel**, group/chair composition validation, opened by **Employee1**.
- Between gate layers (inside lower station) at most **N people** can be present.
- Upper station exit uses **2 one-way lanes** in parallel.
- Lower station is operated by **Employee1**, upper station by **Employee2**.
- In danger situations, Employee1 or Employee2 can **stop the lift** (*signal1*).
- To **resume**, the stopping employee communicates with the other; after a readiness acknowledgment, the lift restarts (*signal2*).
- Bikers ride down one of 3 trails with different average ride times: **T1 < T2 < T3**.
- Operating hours: **Tp–Tk**:
  - at time **Tk**, gates stop accepting passes,
  - everyone already on the platform must still be transported uphill,
  - then after **3 seconds** the lift must shut down.
- Children aged **4–8** must ride with an adult:
  - one adult may supervise **up to 2** children (age 4–8).
- Every gate pass usage is logged: **pass_id – timestamp**.
- VIP users (~**1%**) enter the lower station **without queueing** (still using a pass).
- At the end of the day, a report is generated with the number of rides per pass/person.

## High-Level Architecture

The simulation is implemented as multiple cooperating processes (and optionally helper threads), communicating via IPC:

- **Cashier** – sells/validates passes, assigns `pass_id`, applies discounts.
- **Employee1 (Lower Station)** – manages platform entry gates and validates chair group composition, can stop/resume the lift.
- **Employee2 (Upper Station)** – manages upper station flow, can stop/resume the lift, acknowledges readiness to restart.
- **Tourist** – simulates a single person (pedestrian/biker, VIP/normal, age, etc.). Some tourists may decide not to ride.

### IPC & Synchronization (typical setup)

This project uses classic UNIX IPC primitives:
- **Message queues** for request/response communication between tourists and cashier/employees.
- **Semaphores / shared memory / mutexes** (depending on implementation) to enforce:
  - max **N people** inside the lower station
  - gate parallelism constraints (4 entry gates, 3 platform gates, 2 exit lanes)
  - max **36 occupied chairs** at once
  - lift stop/resume state and employee handshakes
- **Logging** into text files for the simulation report.


## Main Rules Enforced in the Simulation

- **Capacity constraints**
  - Lower station interior limited to **N** people.
  - No more than **36** simultaneously occupied chairs.
- **Chair composition validation**
  - 2 bikers OR 1 biker + 2 pedestrians OR 4 pedestrians.
- **Discount policy**
  - 25% for age < 10 and age > 65.
- **Child supervision**
  - ages 4–8 must be assigned to an adult supervisor.
  - max 2 supervised children (4–8) per adult.
- **VIP handling**
  - VIP tourists bypass the queue at lower station entry (still validated and logged).
- **Operating hours**
  - after time **Tk** passes are rejected at gates.
  - platform occupants must still ride up.
  - lift shuts down 3 seconds after platform drains.

