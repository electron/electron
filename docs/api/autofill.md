# autofill

> add/get/remove profile or credit card

Example

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

const ses = win.webContents.session

const guid = ses.autofill.addCreditCard({
      name: 'example card',
      card_number: '1234567890',
      expiration_month: '12',
      expiration_year: '2022'
})

const card = ses.autofill.getCreditCard(guid)
console.log(card)

ses.autofill.removeCreditCard(guid)
```


## Methods

The `autofill` module has the following methods:

### `autofill.addProfile(profile)`

* `profile` Object
  * `full_name` string
  * `company_name` string
  * `street_address` string
  * `city` string
  * `state` string
  * `locality` string
  * `postal_code` string
  * `sorting_code` string
  * `country_code` string
  * `phone` string
  * `email` string
  * `language_code` string
* `guid` string

Add `profile` object to database and return `guid` reference to it.

### `autofill.getProfile(guid)`

Returns `profile` object by `guid`.

### `autofill.removeProfile(guid)`

Removes `profile` object by `guid`.

### `autofill.addCreditCard(card)`

* `card` Object
  * `name` string
  * `card_number` string
  * `expiration_month` string
  * `expiration_year` string
* `guid` string

Add `card` object to database and return `guid` reference to it.

### `autofill.getCreditCard(guid)`

Returns `card` object by `guid`.

### `autofill.removeCreditCard(guid)`

Removes `card` object by `guid`.
