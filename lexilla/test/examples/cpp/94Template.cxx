// Test JavaScript template expressions for issue 94

// Basic
var basic = `${identifier}`;

// Nested
var nested = ` ${ ` ${ 1 } ` } `;

// With escapes
var xxx = {
    '1': `\`\u0020\${a${1 + 1}b}`,
    '2': `\${a${ `b${1 + 2}c`.charCodeAt(2) }d}`,
    '3': `\${a${ `b${ `c${ JSON.stringify({
        '4': {},
        }) }d` }e` }f}`,
};

// Original request
fetchOptions.body = `
{
	"accountNumber" : "248796",
	"customerType" : "Shipper",
	"destinationCity" : "${order.destination.city}",
	"destinationState" : "${order.destination.stateProvince}",
	"destinationZip" : ${order.destination.postalCode},
	"paymentType" : "Prepaid",
	"shipmentInfo" :
	{
		"items" : [ { "shipmentClass" : "50", "shipmentWeight" : "${order.totalWeight.toString()}" } ]
	}
}`;
